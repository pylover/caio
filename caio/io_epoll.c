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
#include <errno.h>
#include <stdlib.h>

#include "caio/io_epoll.h"


int
caio_io_epoll_init(struct caio_io_epoll *e, size_t maxevents) {
    if (e == NULL) {
        return -1;
    }

    e->maxevents = maxevents;
    e->events = calloc(maxevents, sizeof(struct epoll_event));
    if (e->events == NULL) {
        return -1;
    }

    /* Create e instance */
    e->fd = epoll_create1(0);
    if (e->fd < 0) {
        free(e->events);
        return -1;
    }

    return 0;
}


int
caio_io_epoll_deinit(struct caio_io_epoll *e) {
    if (e == NULL) {
        return -1;
    }

    if (e->fd != -1) {
        close(e->fd);
        e->fd = -1;
    }

    if (e->events) {
        free(e->events);
    }

    return 0;
}


int
caio_io_epoll_monitor(struct caio_io_epoll *e, struct caio_task *task, int fd,
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

    return 0;
}


int
caio_io_epoll_forget(struct caio_io_epoll *e, int fd) {
    if (epoll_ctl(e->fd, EPOLL_CTL_DEL, fd, NULL)) {
        return -1;
    }

    return 0;
}


int
caio_io_epoll_wait(struct caio_io_epoll *e, int timeout) {
    int nfds;
    int i;
    struct caio_task *task;

    /* TODO: Increase e->maxevents if it's smaller than count. */
    nfds = epoll_wait(e->fd, e->events, e->maxevents, timeout);
    if (nfds < 0) {
        return -1;
    }

    if (nfds == 0) {
        return 0;
    }

    for (i = 0; i < nfds; i++) {
        task = (struct caio_task*)e->events[i].data.ptr;
        if (task->status == CAIO_WAITINGEPOLL) {
            task->status = CAIO_RUNNING;
        }
    }

    return 0;
}
