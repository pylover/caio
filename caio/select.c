// Copyright 2023 Vahid Mardani
/*
 * This file is part of caio.
 *  caio is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  caio is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with caio. If not, see <https://www.gnu.org/licenses/>.
 *
 *  Author: Vahid Mardani <vahid.mardani@gmail.com>
 */
#include <stdlib.h>
#include <string.h>
#ifndef CAIO_FDMON_MAXFILES
#include <sys/resource.h>
#endif

#include "caio/select.h"


#define FILEEVENT_RESET(fe) \
            (fe)->task = NULL; \
            (fe)->fd = -1; \
            (fe)->events = 0


struct caio_fileevent {
    int fd;
    int events;
    struct caio_task *task;
};


struct caio_select {
    struct caio_fdmon;
    unsigned int maxfileno;
    size_t waitingfiles;
    struct caio_fileevent *events;
    size_t eventscount;
};


static int
_tick(struct caio *c, struct caio_select *s, unsigned int timeout_us) {
    int i;
    int fd;
    int nfds;
    int shift;
    struct caio_fileevent *fe;
    struct timeval tv;
    fd_set rfds;
    fd_set wfds;
    fd_set efds;

    if (s->waitingfiles == 0) {
        return 0;
    }

    tv.tv_usec = timeout_us % 1000000;
    tv.tv_sec = timeout_us / 1000000;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);
    for (i = 0; i < s->eventscount; i++) {
        fe = &s->events[i];
        fd = fe->fd;

        if (fe->events == 0) {
            s->waitingfiles--;
            continue;
        }

        if (fe->events & CAIO_IN) {
            FD_SET(fd, &rfds);
        }

        if (fe->events & CAIO_OUT) {
            FD_SET(fd, &wfds);
        }

        if (fe->events & CAIO_ERR) {
            FD_SET(fd, &efds);
        }
    }

    nfds = select(s->maxfileno + 1, &rfds, &wfds, &efds, &tv);
    if (nfds == -1) {
        return -1;
    }

    if (nfds == 0) {
        return 0;
    }

    shift = 0;
    for (i = 0; i < s->eventscount; i++) {
        fe = &s->events[i];
        fd = fe->fd;

        if ((fd == -1)
                || FD_ISSET(fd, &rfds)
                || FD_ISSET(fd, &wfds)
                || FD_ISSET(fd, &efds)) {
            if (fe->task && (fe->task->status == CAIO_WAITING)) {
                fe->task->status = CAIO_RUNNING;
                s->waitingfiles--;
                FILEEVENT_RESET(fe);
            }
            shift++;
            continue;
        }

        if (!shift) {
            continue;
        }

        s->events[i - shift] = *fe;
        FILEEVENT_RESET(fe);
    }
    s->eventscount = s->waitingfiles;
    return 0;
}


static int
_monitor(struct caio_select *s, struct caio_task *task, int fd, int events) {
    struct caio_fileevent *fe;
    if ((fd < 0) || (fd > s->maxfileno) || (s->eventscount == s->maxfileno)) {
        return -1;
    }

    fe = &s->events[s->eventscount++];
    s->waitingfiles++;
    fe->events = events;
    fe->task = task;
    fe->fd = fd;
    return 0;
}


static int
_forget(struct caio_select *s, int fd) {
    int i;
    struct caio_fileevent *fe;

    for (i = 0; i < s->eventscount; i++) {
        fe = &s->events[i];
        if (fe->fd == fd) {
            FILEEVENT_RESET(fe);
            s->waitingfiles--;
            return 0;
        }
    }

    return -1;
}


struct caio_select *
caio_select_create(struct caio* c, size_t maxevents) {
    struct caio_select *s;

    /* Create select instance */
    s = malloc(sizeof(struct caio_select));
    if (s == NULL) {
        return NULL;
    }
    memset(s, 0, sizeof(struct caio_select));

    s->waitingfiles = 0;
    s->tick = (caio_tick) _tick;
    s->monitor = (caio_filemonitor)_monitor;
    s->forget = (caio_fileforget)_forget;

    if (caio_module_install(c, (struct caio_module*)s)) {
        goto failed;
    }

#ifndef CAIO_FDMON_MAXFILES
    /* Find maximum allowed file descriptors for this process and allocate
     * as much as needed for task repository
     */
    struct rlimit limits;
    if (getrlimit(RLIMIT_NOFILE, &limits)) {
        goto failed;
    }

    if (maxevents > limits.rlim_max) {
        goto failed;
    }
#else
    if (maxevents > CAIO_FDMON_MAXFILES) {
        goto failed;
    }
#endif
    /* select(2) requires the highest number of fileno instead of event count.
     * So, it must increased 3 times for (stdin, stdout and stderr) */
    s->maxfileno = maxevents + 3;
    s->events = calloc(s->maxfileno, sizeof(struct caio_fileevent));
    s->eventscount = 0;
    if (s->events == NULL) {
        goto failed;
    }
    return s;

failed:
    free(s);
    return NULL;
}


int
caio_select_destroy(struct caio* c, struct caio_select *s) {
    int ret = 0;

    if (s == NULL) {
        return -1;
    }

    ret |= caio_module_uninstall(c, (struct caio_module*)s);

    if (s->events) {
        free(s->events);
    }

    free(s);
    return 0;
}
