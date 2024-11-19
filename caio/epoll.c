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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>

#include "caio/caio.h"
#include "caio/fdmon.h"
#include "caio/epoll.h"


struct caio_epoll {
    struct caio_fdmon;
    int fd;
    size_t maxevents;
    size_t waitingfiles;
    struct epoll_event *events;
};


static int
_tick(struct caio *c, struct caio_epoll *e, unsigned int timeout_us) {
    int i;
    int nfds;
    struct caio_task *task;

    if (e->waitingfiles == 0) {
        return 0;
    }

    errno = 0;
    nfds = epoll_wait(e->fd, e->events, e->maxevents, timeout_us / 1000);
    if (nfds < 0) {
        return -1;
    }

    if (nfds) {
        for (i = 0; i < nfds; i++) {
            task = (struct caio_task*)e->events[i].data.ptr;
            if (task->status == CAIO_WAITING) {
                task->status = CAIO_RUNNING;
                e->waitingfiles--;
            }
        }
    }

    fdmon_tasks_timeout_check(c);
    return 0;
}


static int
_monitor(struct caio_epoll *e, struct caio_task *task, int fd,
        int events, unsigned int timeout_us) {
    struct epoll_event ee;

    ee.events = events | EPOLLONESHOT;
    ee.data.ptr = task;
    if (epoll_ctl(e->fd, EPOLL_CTL_MOD, fd, &ee)) {
        if (epoll_ctl(e->fd, EPOLL_CTL_ADD, fd, &ee)) {
            return -1;
        }
        errno = 0;
    }

    e->waitingfiles++;
    task->fdmon_timeout_us = timeout_us;
    clock_gettime(CLOCK_MONOTONIC, &task->fdmon_timestamp);
    return 0;
}


static int
_forget(struct caio_epoll *e, int fd) {
    if (epoll_ctl(e->fd, EPOLL_CTL_DEL, fd, NULL)) {
        return -1;
    }

    return 0;
}


struct caio_epoll *
caio_epoll_create(struct caio* c, size_t maxevents) {
    struct caio_epoll *e;

    if (maxevents == 0) {
        return NULL;
    }

    /* Create epoll instance */
    e = malloc(sizeof(struct caio_epoll));
    if (e == NULL) {
        return NULL;
    }
    memset(e, 0, sizeof(struct caio_epoll));

    e->waitingfiles = 0;
    e->maxevents = maxevents;
    e->fd = epoll_create1(0);
    if (e->fd < 0) {
        goto failed;
    }

    e->events = calloc(e->maxevents, sizeof(struct epoll_event));
    if (e->events == NULL) {
        goto failed;
    }

    e->tick = (caio_tick) _tick;
    e->monitor = (caio_filemonitor)_monitor;
    e->forget = (caio_fileforget)_forget;

    if (caio_module_install(c, (struct caio_module*)e)) {
        goto failed;
    }

    return e;

failed:
    if (e->events) {
        free(e->events);
    }

    free(e);
    return NULL;
}


int
caio_epoll_destroy(struct caio* c, struct caio_epoll *e) {
    int ret = 0;

    if (e == NULL) {
        return -1;
    }

    ret |= caio_module_uninstall(c, (struct caio_module*)e);

    if (e->fd != -1) {
        close(e->fd);
    }

    if (e->events) {
        free(e->events);
    }

    free(e);
    return ret;
}
