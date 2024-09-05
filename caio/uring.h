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


#define CAIO_URING_AWAIT(umod, task, taskcount) \
    do { \
        (task)->current->line = __LINE__; \
        if ((task)->uring && \
                (caio_uring_task_waitingjobs(task) < \
                 (taskcount))) { \
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


int
caio_uring_cqe_seen(struct caio_uring *u, struct caio_task *task, int index);


struct io_uring_sqe *
caio_uring_sqe_get(struct caio_uring *u, struct caio_task *task);


int
caio_uring_task_waitingjobs(struct caio_task *task);


int
caio_uring_task_completed(struct caio_task *task);


int
caio_uring_task_cleanup(struct caio_uring *u, struct caio_task *task);


struct io_uring_cqe *
caio_uring_cqe_get(struct caio_task *task, int index);


#define caio_uring_submit(umod) io_uring_submit(&(u)->ring)
#define caio_uring_prep_read io_uring_prep_read
#define caio_uring_prep_write io_uring_prep_write
#define caio_uring_prep_readv io_uring_prep_readv
#define caio_uring_prep_writev io_uring_prep_writev
#define caio_uring_prep_socket io_uring_prep_socket
#define caio_uring_prep_accept io_uring_prep_accept
#define caio_uring_prep_accept_multishot io_uring_prep_multishot_accept


int
caio_uring_read(struct caio_uring *u, struct caio_task *task, int fd,
        void *buf, unsigned nbytes, __u64 offset);


int
caio_uring_write(struct caio_uring *u, struct caio_task *task, int fd,
        void *buf, unsigned nbytes, __u64 offset);


int
caio_uring_readv(struct caio_uring *u, struct caio_task *task, int fd,
        const struct iovec *iovecs, unsigned nrvecs, __u64 offset);


int
caio_uring_writev(struct caio_uring *u, struct caio_task *task, int fd,
        const struct iovec *iovecs, unsigned nrvecs, __u64 offset);


int
caio_uring_socket(struct caio_uring *u, struct caio_task *task, int domain,
        int type, int protocol, unsigned int flags);


int
caio_uring_accept(struct caio_uring *u, struct caio_task *task, int sockfd,
        struct sockaddr *addr, socklen_t *addrlen, unsigned int flags);


int
caio_uring_accept_multishot(struct caio_uring *u, struct caio_task *task,
        int sockfd, struct sockaddr *addr, socklen_t *addrlen,
        unsigned int flags);


#endif  // CAIO_URING_H_
