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
#ifndef CAIO_EPOLL_H_
#define CAIO_EPOLL_H_


#include "caio/caio.h"


struct caio_epoll {
    int fd;
    struct epoll_event *events;
    size_t maxevents;
};


int
caio_epoll_init(struct caio_epoll *e, size_t maxevents);


int
caio_epoll_deinit(struct caio_epoll *io);


int
caio_epoll_monitor(struct caio_epoll *e, struct caio_task *task, int fd,
        int events);


int
caio_epoll_forget(struct caio_epoll *e, int fd);


int
caio_epoll_wait(struct caio_epoll *e, int timeout);


#endif  // CAIO_EPOLL_H_
