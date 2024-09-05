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

#include <clog.h>

#include "caio/uring.h"


struct caio_uring {
    struct caio_module;
    struct io_uring ring;

    sigset_t *sigmask;
    int timeout_ms;
    unsigned int jobsmax;

    unsigned int jobstotal;
    unsigned int jobswaiting;
};


struct caio_uring_taskstate {
    volatile unsigned int waiting;
    volatile unsigned int completed;
    struct io_uring_cqe *cqes[CAIO_URING_TASK_MAXWAITING];
};


struct io_uring_sqe *
caio_uring_sqe_get(struct caio_uring *u, struct caio_task *task) {
    struct caio_uring_taskstate *ustate = task->uring;

    if (u->jobstotal >= u->jobsmax) {
        return NULL;
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(&(u)->ring);
    if (sqe == NULL) {
        return NULL;
    }

    io_uring_sqe_set_data(sqe, task);
    if (ustate == NULL) {
        ustate = malloc(sizeof(struct caio_uring_taskstate));
        if (ustate == NULL) {
            return NULL;
        }
        ustate->waiting = 0;
        ustate->completed = 0;
        task->uring = ustate;
    }

    if (ustate->waiting >= CAIO_URING_TASK_MAXWAITING) {
        return NULL;
    }

    ustate->waiting++;
    u->jobstotal++;
    u->jobswaiting++;
    return sqe;
}


static int
_tick(struct caio_uring *u, struct caio* c) {
    struct io_uring_cqe *cqe;
    struct caio_task *task;
    struct caio_uring_taskstate *ustate;

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
    int ret = io_uring_wait_cqes(&u->ring, &cqe, u->jobswaiting, &timeout,
            u->sigmask);
    if (ret < 0) {
        if (ret == -ETIME) {
            return 0;
        }
        errno = abs(ret);
        return -1;
    }

    u->jobswaiting--;
    task = (struct caio_task *) io_uring_cqe_get_data(cqe);
    ustate = task->uring;
    if (ustate == NULL) {
        return -1;
    }

    if (ustate->waiting == 0) {
        /* weird situation! */
        return -1;
    }

    ustate->waiting--;
    ustate->cqes[ustate->completed++] = cqe;

    if (ustate->waiting) {
        return 0;
    }

    if (task->status == CAIO_WAITING) {
        task->status = CAIO_RUNNING;
    }

    return 0;
}


int
caio_uring_cqe_seen(struct caio_uring *u, struct caio_task *task, int index) {
    struct caio_uring_taskstate *ustate = task->uring;
    struct io_uring_cqe *cqe;

    if (ustate == NULL) {
        return -1;
    }

    if (index == ustate->completed) {
        return -1;
    }

    cqe = ustate->cqes[index];
    if (cqe == NULL) {
        return -1;
    }

    io_uring_cqe_seen(&u->ring, cqe);
    ustate->completed--;
    u->jobstotal--;

    if (ustate->completed == 0) {
        free(ustate);
        task->uring = NULL;
    }

    return 0;
}


struct io_uring_cqe *
caio_uring_cqe_get(struct caio_task *task, int index) {
    struct caio_uring_taskstate *ustate = task->uring;

    if (ustate == NULL) {
        return NULL;
    }

    if (index == ustate->completed) {
        return NULL;
    }

    return ustate->cqes[index];
}


struct caio_uring *
caio_uring_create(struct caio* c, unsigned int jobsmax,
        unsigned int timeout_ms, sigset_t *sigmask) {
    struct caio_uring *u;

    if (jobsmax == 0) {
        return NULL;
    }

    /* Create uring instance */
    u = malloc(sizeof(struct caio_uring));
    if (u == NULL) {
        return NULL;
    }
    memset(u, 0, sizeof(struct caio_uring));

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

    if (caio_module_install(c, (struct caio_module*)u)) {
        free(u);
        return NULL;
    }

    return u;
}


int
caio_uring_destroy(struct caio* c, struct caio_uring *u) {
    int ret = 0;

    if (u == NULL) {
        return -1;
    }

    io_uring_queue_exit(&u->ring);
    ret |= caio_module_uninstall(c, (struct caio_module*)u);
    free(u);

    return ret;
}


int
caio_uring_task_waitingjobs(struct caio_task *task) {
    struct caio_uring_taskstate *ustate = task->uring;

    if (ustate == NULL) {
        return 0;
    }

    return ustate->waiting;
}


int
caio_uring_task_completed(struct caio_task *task) {
    struct caio_uring_taskstate *ustate = task->uring;

    if (ustate == NULL) {
        return 0;
    }

    return ustate->completed;
}


int
caio_uring_task_cleanup(struct caio_uring *u, struct caio_task *task) {
    struct caio_uring_taskstate *ustate = task->uring;
    int i;

    if (ustate == NULL) {
        return 0;
    }

    for (i = 0; i < ustate->completed; i++) {
        caio_uring_cqe_seen(u, task, i);
    }

    for (i = 0; i < ustate->waiting; i++) {
        u->jobstotal--;
    }

    free(ustate);
    task->uring = NULL;

    return 0;
}


#define _CREATE_PREP_SUBMIT(name, umod, task, ...) \
    struct io_uring_sqe *sqe; \
    sqe = caio_uring_sqe_get(umod, task); \
    if (sqe == NULL) return -1; \
    caio_uring_prep_ ## name(sqe, __VA_ARGS__); \
    return caio_uring_submit(umod);


int
caio_uring_readv(struct caio_uring *u, struct caio_task *task, int fd,
        const struct iovec *iovecs, unsigned nrvecs, __u64 offset) {
    _CREATE_PREP_SUBMIT(readv, u, task, fd, iovecs, nrvecs, offset);
}


int
caio_uring_writev(struct caio_uring *u, struct caio_task *task, int fd,
        const struct iovec *iovecs, unsigned nrvecs, __u64 offset) {
    _CREATE_PREP_SUBMIT(writev, u, task, fd, iovecs, nrvecs, offset);
}


int
caio_uring_socket(struct caio_uring *u, struct caio_task *task, int domain,
        int type, int protocol, unsigned int flags) {
    _CREATE_PREP_SUBMIT(socket, u, task, domain, type, protocol, flags);
}


int
caio_uring_accept(struct caio_uring *u, struct caio_task *task, int sockfd,
        struct sockaddr *addr, socklen_t *addrlen, unsigned int flags) {
    _CREATE_PREP_SUBMIT(accept, u, task, sockfd, addr, addrlen, flags);
}


int
caio_uring_accept_multishot(struct caio_uring *u, struct caio_task *task,
        int sockfd, struct sockaddr *addr, socklen_t *addrlen,
        unsigned int flags) {
    _CREATE_PREP_SUBMIT(accept_multishot, u, task, sockfd, addr, addrlen,
            flags);
}


int
caio_uring_read(struct caio_uring *u, struct caio_task *task, int fd,
        void *buf, unsigned nbytes, __u64 offset) {
    _CREATE_PREP_SUBMIT(read, u, task, fd, buf, nbytes, offset);
}


int
caio_uring_write(struct caio_uring *u, struct caio_task *task, int fd,
        void *buf, unsigned nbytes, __u64 offset) {
    _CREATE_PREP_SUBMIT(write, u, task, fd, buf, nbytes, offset);
}
