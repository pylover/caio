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
#ifndef CAIO_URING_H_
#define CAIO_URING_H_


#include <liburing.h>

#include "caio/caio.h"


struct caio_uring;


#define caio_uring_readv io_uring_prep_readv
#define caio_uring_writev io_uring_prep_writev


#define CAIO_URING_AWAIT(umod, task, taskcount, results) \
    do { \
        (task)->current->line = __LINE__; \
        if (caio_uring_monitor(umod, task, taskcount, results)) { \
            (task)->status = CAIO_TERMINATING; \
        } \
        else { \
            (task)->status = CAIO_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


struct caio_uring *
caio_uring_create(struct caio* c, unsigned int jobsmax,
        unsigned int timeout_ms, sigset_t *sigmask);


int
caio_uring_destroy(struct caio* c, struct caio_uring *u);


void
caio_uring_seen(struct caio_uring *u, struct io_uring_cqe * cqe);


int
caio_uring_monitor(struct caio_uring *u, struct caio_task *task,
        unsigned int jobcount, struct io_uring_cqe **results);


struct io_uring_sqe *
caio_uring_sqe_get(struct caio_uring *u);


int
caio_uring_submit(struct caio_uring *u);


#endif  // CAIO_URING_H_
