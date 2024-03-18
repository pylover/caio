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
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/io_uring.h>

#include "caio/caio.h"
#include "caio/io_uring.h"


/*
 * Thread safe (atmoic) push and pop from ringbuffers
 * https://gcc.gnu.org/wiki/Atomic/GCCMM/AtomicSync
 */
/* Macros for barriers needed by io_uring */
#if defined(__x86_64) || defined(__i386__)
#define read_barrier()  __asm__ __volatile__("":::"memory")
#define write_barrier() __asm__ __volatile__("":::"memory")
#else
#define read_barrier()  __sync_synchronize()
#define write_barrier() __sync_synchronize()
#endif


/*
 * System call wrappers provided since glibc does not yet
 * provide wrappers for io_uring system calls.
 */

static int
io_uring_setup(unsigned entries, struct io_uring_params *p) {
    return (int) syscall(__NR_io_uring_setup, entries, p);
}


static int
io_uring_enter(int ringfd, unsigned int to_submit, unsigned int min_complete,
        unsigned int flags) {
    return (int) syscall(__NR_io_uring_enter, ringfd, to_submit,
                    min_complete, flags, NULL, 0);
}


int
caio_io_uring_init(struct caio_io_uring *u, size_t maxtasks) {
    struct io_uring_params *p = &u->params;

    /* See io_uring_setup(2) for io_uring_params.flags you can set */
    memset(p, 0, sizeof(struct io_uring_params));
    p->flags = IORING_SETUP_NO_SQARRAY;

    u->fd = io_uring_setup(maxtasks, p);
    if (u->fd < 0) {
        return -1;
    }

    /* io_uring communication happens via 2 shared kernel‐user space ring
     * buffers, which can be jointly mapped with a single mmap() call in
     * kernels >= 5.4.
     */
    u->sq.size = p->sq_off.array + p->sq_entries * sizeof(__u32);
    u->cq.size = p->cq_off.cqes + p->cq_entries * sizeof(struct io_uring_cqe);

    /* Rather than check for kernel version, the recommended way is to
     * check the features field of the io_uring_params structure, which is a
     * bitmask. If IORING_FEAT_SINGLE_MMAP is set, we can do away with the
     * second mmap() call to map in the completion ring separately.
     */
    if (p->features & IORING_FEAT_SINGLE_MMAP) {
        if (u->cq.size > u->sq.size)
            u->sq.size = u->cq.size;
        u->cq.size = u->sq.size;
    }

    /* Map in the submission and completion queue ring buffers.
     *  Kernels < 5.4 only map in the submission queue, though.
     */
    u->sq.start = mmap(0, u->sq.size, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_POPULATE,
                  u->fd, IORING_OFF_SQ_RING);
    if (u->sq.start == MAP_FAILED) {
        return -1;
    }

    if (p->features & IORING_FEAT_SINGLE_MMAP) {
        u->cq.start = u->sq.start;
    }
    else {
        /* Map in the completion queue ring buffer in older kernels
         * separately */
        u->cq.start = mmap(0, u->cq.size, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_POPULATE,
                      u->fd, IORING_OFF_CQ_RING);
        if (u->cq.start == MAP_FAILED) {
            return 1;
        }
    }

    /* Save useful fields for later easy reference */
    u->sq.tosubmit = 0;
    u->sq.tail = u->sq.start + p->sq_off.tail;
    u->sq.head = u->sq.start + p->sq_off.head;
    u->sq.mask = u->sq.start + p->sq_off.ring_mask;

    /* Map in the submission queue entries array */
    u->sq.array = mmap(0, p->sq_entries * sizeof(struct io_uring_sqe),
                   PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                   u->fd, IORING_OFF_SQES);
    if (u->sq.array == MAP_FAILED) {
        return 1;
    }

    /* Save useful fields for later easy reference */
    u->cq.head = u->cq.start + p->cq_off.head;
    u->cq.tail = u->cq.start + p->cq_off.tail;
    u->cq.mask = u->cq.start + p->cq_off.ring_mask;
    u->cq.array = u->cq.start + p->cq_off.cqes;

    return 0;
}


int
caio_io_uring_deinit(struct caio_io_uring *u) {
    struct io_uring_params *p = &u->params;

    /* Unmap the submission queue entries array */
    if (u->sq.array && munmap(u->sq.array,
                p->sq_entries * sizeof(struct io_uring_sqe))) {
        goto failed;
    }

    if (munmap(u->sq.start, u->sq.size)) {
        goto failed;
    }

    if (!(p->features & IORING_FEAT_SINGLE_MMAP)) {
        if (munmap(u->sq.start, u->cq.size)) {
            goto failed;
        }
    }

    close(u->fd);
    return 0;

failed:
    close(u->fd);
    return -1;
}


struct io_uring_sqe *
caio_io_uring_sqe_get(struct caio_io_uring *u) {
    if (CAIO_IO_RING_SQ_ISFULL(u->sq)) {
        return NULL;
    }
    read_barrier();
    return &u->sq.array[(*u->sq.tail + u->sq.tosubmit++) & *u->sq.mask];
}


/** Add the submission queue entries to the tail of the SQE ring buffer
 */
int
caio_io_uring_sqe_submit(struct caio_io_uring *u) {
    unsigned int tosubmit = u->sq.tosubmit;
    int submitted;

    if (!tosubmit) {
        return 0;
    }

    /* Update the submission queue's tail with atmoic operation.
     */
    write_barrier();
    *u->sq.tail = *u->sq.tail + tosubmit;
    u->sq.tosubmit = 0;
    write_barrier();

    /* Tell the kernel we have submitted events with the io_uring_enter()
     * system call.
     */
    submitted = io_uring_enter(u->fd, tosubmit, 0, 0);
    if (submitted < 0) {
        return -1;
    }

    tosubmit -= submitted;
    if (tosubmit) {
        return -1;
    }

    return 0;
}


int
caio_io_uring_cq_wait(struct caio_io_uring *u) {
    /* Wait for at least one task to complete by pass in the
     * IOURING_ENTER_GETEVENTS flag which causes the io_uring_enter() call to
     * wait until min_complete (the 3rd param) events complete.
     */
    return io_uring_enter(u->fd, 0, 1, IORING_ENTER_GETEVENTS);
}


int
caio_io_uring_cq_check(struct caio_io_uring *u) {
    struct caio_io_uring_cq *cq = &u->cq;
    struct io_uring_cqe *cqe;
    unsigned int head;
    unsigned int tail;
    unsigned int mask = cq->mask;

    read_barrier();
    head = *cq->head;
    tail = *cq->tail;

    while (head == tail) {
        cqe = u->cq.array[head & mask];
        // TODO: consume cqe
        *cq->head = ++head;
        write_barrier();
    }

    return 0;
}
