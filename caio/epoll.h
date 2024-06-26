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


#include <sys/epoll.h>

#include <caio/caio.h>


typedef struct caio_epoll *caio_epoll_t;


caio_epoll_t
caio_epoll_create(caio_t c, size_t maxevents, unsigned int timeout);


int
caio_epoll_destroy(caio_t c, caio_epoll_t e);


int
caio_epoll_monitor(caio_epoll_t e, struct caio_task *task, int fd, int events);


int
caio_epoll_forget(caio_epoll_t e, int fd);


#define CAIO_AWAIT_EPOLL(module, task, ...) \
    CAIO_AWAIT_MODULE(caio_epoll, module, task, __VA_ARGS__)


#endif  // CAIO_EPOLL_H_
