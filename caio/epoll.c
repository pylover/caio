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
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "caio/epoll.h"


struct caio_epoll {
    struct caio_module;
    int fd;
    int timeout_ms;
    size_t maxevents;
    size_t waitingfiles;
    struct epoll_event *events;
};


static int
_tick(struct caio_epoll *e, caio_t c) {
    int i;
    int nfds;
    struct caio_task *task;

    if (e->waitingfiles == 0) {
        return 0;
    }

    nfds = epoll_wait(e->fd, e->events, e->maxevents, e->timeout_ms);
    if (nfds < 0) {
        return -1;
    }

    if (nfds == 0) {
        return 0;
    }

    for (i = 0; i < nfds; i++) {
        task = (struct caio_task*)e->events[i].data.ptr;
        if (task->status == CAIO_WAITING) {
            task->status = CAIO_RUNNING;
            e->waitingfiles--;
        }
    }

    return 0;
}


struct caio_epoll *
caio_epoll_create(caio_t c, size_t maxevents, unsigned int timeout_ms) {
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

    e->timeout_ms = timeout_ms;
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

    e->tick = (caio_hook) _tick;

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
caio_epoll_destroy(caio_t c, struct caio_epoll *e) {
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
    return 0;
}


int
caio_epoll_monitor(struct caio_epoll *e, struct caio_task *task, int fd,
        int events) {
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
    return 0;
}


int
caio_epoll_forget(struct caio_epoll *e, int fd) {
    if (epoll_ctl(e->fd, EPOLL_CTL_DEL, fd, NULL)) {
        return -1;
    }

    return 0;
}
