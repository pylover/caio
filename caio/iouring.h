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
#ifndef CAIO_IOURING_H_
#define CAIO_IOURING_H_


#include <liburing.h>

#include "caio/caio.h"


struct caio_iouring;


#define CAIO_IOURING_AWAIT(module, task, taskcount, results) \
    do { \
        (task)->current->line = __LINE__; \
        if (caio_iouring_monitor(module, task, taskcount, results)) { \
            (task)->status = CAIO_TERMINATING; \
        } \
        else { \
            (task)->status = CAIO_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


struct caio_iouring *
caio_iouring_create(struct caio* c, unsigned int jobsmax,
        unsigned int timeout_ms, sigset_t *sigmask);


int
caio_iouring_destroy(struct caio* c, struct caio_iouring *u);


void
caio_iouring_seen(struct caio_iouring *u, struct io_uring_cqe * cqe);


int
caio_iouring_monitor(struct caio_iouring *u, struct caio_task *task,
        unsigned int jobcount, struct io_uring_cqe **results);


int
caio_iouring_readv(struct caio_iouring *u, int fd,
        const struct iovec *iovecs, unsigned nr_vecs, __u64 offset);


int
caio_iouring_writev(struct caio_iouring *u, int fd,
        const struct iovec *iovecs, unsigned nr_vecs, __u64 offset);


#endif  // CAIO_IOURING_H_
