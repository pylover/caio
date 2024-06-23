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
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>

#include "caio/select.h"


struct caio_select {
    struct caio_module;
    int timeout;
    size_t waitingfiles;
    fd_set rfds;
    fd_set wfds;
    fd_set efds;
    struct caio_task *tasks[FD_SETSIZE];
};


static int
_tick(struct caio_select *s, caio_t c) {
    // int i;
    // int nfds;
    // struct caio_task *task;

    if (s->waitingfiles == 0) {
        return 0;
    }

    // nfds = epoll_wait(s->fd, s->events, s->maxevents, s->timeout);
    // if (nfds < 0) {
    //     return -1;
    // }

    // if (nfds == 0) {
    //     return 0;
    // }

    // for (i = 0; i < nfds; i++) {
    //     task = (struct caio_task*)s->events[i].data.ptr;
    //     if (task->status == CAIO_WAITING) {
    //         task->status = CAIO_RUNNING;
    //         s->waitingfiles--;
    //     }
    // }

    return 0;
}


struct caio_select *
caio_select_create(caio_t c, unsigned int timeout) {
    struct caio_select *s;

    /* Create select instance */
    s = malloc(sizeof(struct caio_select));
    if (s == NULL) {
        return NULL;
    }
    memset(s, 0, sizeof(struct caio_select));

    s->timeout = timeout;
    s->waitingfiles = 0;

    FD_ZERO(&s->rfds);
    FD_ZERO(&s->wfds);
    FD_ZERO(&s->efds);

    s->tick = (caio_hook) _tick;

    if (caio_module_install(c, (struct caio_module*)s)) {
        goto failed;
    }

    return s;

failed:
    free(s);
    return NULL;
}


int
caio_select_destroy(caio_t c, caio_select_t s) {
    int ret = 0;

    if (s == NULL) {
        return -1;
    }

    ret |= caio_module_uninstall(c, (struct caio_module*)s);

    free(s);
    return 0;
}


int
caio_select_monitor(struct caio_select *s, struct caio_task *task, int fd,
        int events) {
    if ((fd < 0) || (fd > 1023)) {
        return -1;
    }

    s->tasks[fd] = task;
    if (events & CAIO_READ) {
        FD_SET(fd, &s->rfds);
    }
    if (events & CAIO_WRITE) {
        FD_SET(fd, &s->wfds);
    }
    if (events & CAIO_ERR) {
        FD_SET(fd, &s->efds);
    }

    s->waitingfiles++;
    return 0;
}


int
caio_select_forget(struct caio_select *s, int fd) {
    if (FD_ISSET(fd, &s->rfds)) {
        FD_CLR(fd, &s->rfds);
    }

    if (FD_ISSET(fd, &s->wfds)) {
        FD_CLR(fd, &s->wfds);
    }

    if (FD_ISSET(fd, &s->efds)) {
        FD_CLR(fd, &s->efds);
    }
    return 0;
}
