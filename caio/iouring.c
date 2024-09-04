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

#include "caio/iouring.h"


struct caio_iouring {
    struct caio_module;
    struct io_uring ring;

    sigset_t *sigmask;
    int timeout_ms;
    unsigned int jobsmax;

    unsigned int jobstotal;
    unsigned int jobswaiting;
    struct io_uring_cqe **results;
    struct caio_task *task;
};


int
caio_iouring_readv(struct caio_iouring *u, int fd,
        const struct iovec *iovecs, unsigned nr_vecs, __u64 offset) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&(u)->ring);
    if (sqe == NULL) {
        return -1;
    }

    io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);

    // io_uring_sqe_set_data(sqe, userptr);
    int ret = io_uring_submit(&(u)->ring);
    if (ret < 0) {
        errno = abs(ret);
        return -1;
    }

    u->jobstotal++;
    return 0;
}


int
caio_iouring_writev(struct caio_iouring *u, int fd,
        const struct iovec *iovecs, unsigned nr_vecs, __u64 offset) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&(u)->ring);
    if (sqe == NULL) {
        return -1;
    }

    io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);

    // io_uring_sqe_set_data(sqe, userptr);
    int ret = io_uring_submit(&(u)->ring);
    if (ret < 0) {
        errno = abs(ret);
        return -1;
    }

    u->jobstotal++;
    return 0;
}


static int
_tick(struct caio_iouring *u, struct caio* c) {
    if (u->jobswaiting == 0) {
        return 0;
    }

    if (u->jobswaiting > u->jobstotal) {
        return -1;
    }

    struct __kernel_timespec timeout = {
        .tv_sec = u->timeout_ms / 1000,
        .tv_nsec = (u->timeout_ms % 1000) * 1000,
    };
    int ret = io_uring_wait_cqes(&u->ring, u->results, u->jobswaiting,
            &timeout, u->sigmask);
    if (ret < 0) {
        if (ret == -ETIME) {
            return 0;
        }
        errno = abs(ret);
        return -1;
    }

    if (u->task->status == CAIO_WAITING) {
        u->task->status = CAIO_RUNNING;
        u->jobswaiting = 0;
    }

    return 0;
}


void
caio_iouring_seen(struct caio_iouring *u, struct io_uring_cqe *cqe) {
    if (cqe == NULL) {
        return;
    }
    io_uring_cqe_seen(&u->ring, cqe);
}


struct caio_iouring *
caio_iouring_create(struct caio* c, unsigned int jobsmax,
        unsigned int timeout_ms, sigset_t *sigmask) {
    struct caio_iouring *u;

    if (jobsmax == 0) {
        return NULL;
    }

    /* Create iouring instance */
    u = malloc(sizeof(struct caio_iouring));
    if (u == NULL) {
        return NULL;
    }
    memset(u, 0, sizeof(struct caio_iouring));

    if (io_uring_queue_init(jobsmax, &u->ring, 0) < 0) {
        free(u);
        return NULL;
    }

    u->sigmask = sigmask;
    u->timeout_ms = timeout_ms;
    u->jobsmax = jobsmax;
    u->tick = (caio_hook) _tick;

    u->jobstotal = 0;
    u->jobswaiting = 0;
    u->results = NULL;
    u->task = NULL;

    if (caio_module_install(c, (struct caio_module*)u)) {
        free(u);
        return NULL;
    }

    return u;
}


int
caio_iouring_monitor(struct caio_iouring *u, struct caio_task *task,
        unsigned int jobcount, struct io_uring_cqe **results) {
    if (jobcount > u->jobstotal) {
        return -1;
    }

    u->task = task;
    u->jobswaiting = jobcount;
    u->results = results;
    return 0;
}


int
caio_iouring_destroy(struct caio* c, struct caio_iouring *u) {
    int ret = 0;

    if (u == NULL) {
        return -1;
    }

    io_uring_queue_exit(&u->ring);
    ret |= caio_module_uninstall(c, (struct caio_module*)u);
    free(u);

    return ret;
}
