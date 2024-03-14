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
 * System call wrappers provided since glibc does not yet
 * provide wrappers for io_uring system calls.
 */

static int
io_uring_setup(unsigned entries, struct io_uring_params *p) {
    return (int) syscall(__NR_io_uring_setup, entries, p);
}


// static int
// io_uring_enter(int ringfd, unsigned int to_submit, unsigned int min_complete,
//         unsigned int flags) {
//     return (int) syscall(__NR_io_uring_enter, ringfd, to_submit,
//                     min_complete, flags, NULL, 0);
// }


/*
 * Thread safe (atmoic) push and pop from ringbuffers
 * https://gcc.gnu.org/wiki/Atomic/GCCMM/AtomicSync
 */
/* Macros for barriers needed by io_uring */
#define _ATOMIC_STORE(p, v) \
    atomic_store_explicit((_Atomic typeof(*(p)) *)(p), (v), \
            memory_order_release)
#define _ATOMIC_LOAD(p) \
    atomic_load_explicit((_Atomic typeof(*(p)) *)(p), memory_order_acquire)


int
caio_io_uring_init(struct caio_io_uring *u, size_t maxtasks) {
    struct io_uring_params *p = &u->params;

    /* See io_uring_setup(2) for io_uring_params.flags you can set */
    memset(p, 0, sizeof(struct io_uring_params));
    u->fd = io_uring_setup(maxtasks, p);
    if (u->fd < 0) {
        return -1;
    }

    /*
     * io_uring communication happens via 2 shared kernel‐user space ring
     * buffers, which can be jointly mapped with a single mmap() call in
     * kernels >= 5.4.
     */
    u->sq_len = p->sq_off.array + p->sq_entries * sizeof(__u32);
    u->cq_len = p->cq_off.cqes + p->cq_entries * sizeof(struct io_uring_cqe);

    /* Rather than check for kernel version, the recommended way is to
     * check the features field of the io_uring_params structure, which is a
     * bitmask. If IORING_FEAT_SINGLE_MMAP is set, we can do away with the
     * second mmap() call to map in the completion ring separately.
     */
    if (p->features & IORING_FEAT_SINGLE_MMAP) {
        if (u->cq_len > u->sq_len)
            u->sq_len = u->cq_len;
        u->cq_len = u->sq_len;
    }

    /* Map in the submission and completion queue ring buffers.
     *  Kernels < 5.4 only map in the submission queue, though.
     */
    u->sq_ptr = mmap(0, u->sq_len, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_POPULATE,
                  u->fd, IORING_OFF_SQ_RING);
    if (u->sq_ptr == MAP_FAILED) {
        return -1;
    }

    if (p->features & IORING_FEAT_SINGLE_MMAP) {
        u->cq_ptr = u->sq_ptr;
    }
    else {
        /* Map in the completion queue ring buffer in older kernels
         * separately */
        u->cq_ptr = mmap(0, u->cq_len, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_POPULATE,
                      u->fd, IORING_OFF_CQ_RING);
        if (u->cq_ptr == MAP_FAILED) {
            return 1;
        }
    }

    /* Save useful fields for later easy reference */
    u->sq_tail = u->sq_ptr + p->sq_off.tail;
    u->sq_mask = u->sq_ptr + p->sq_off.ring_mask;
    u->sq_array = u->sq_ptr + p->sq_off.array;

    /* Map in the submission queue entries array */
    u->sqes = mmap(0, p->sq_entries * sizeof(struct io_uring_sqe),
                   PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                   u->fd, IORING_OFF_SQES);
    if (u->sqes == MAP_FAILED) {
        return 1;
    }

    // /* Save useful fields for later easy reference */
    u->cq_head = u->cq_ptr + p->cq_off.head;
    u->cq_tail = u->cq_ptr + p->cq_off.tail;
    u->cq_mask = u->cq_ptr + p->cq_off.ring_mask;
    u->cqes = u->cq_ptr + p->cq_off.cqes;

    return 0;
}


int
caio_io_uring_deinit(struct caio_io_uring *u) {
    struct io_uring_params *p = &u->params;

    /* Unmap the submission queue entries array */
    if (u->sqes && munmap(u->sqes,
                p->sq_entries * sizeof(struct io_uring_sqe))) {
        goto failed;
    }

    if (munmap(u->sq_ptr, u->sq_len)) {
        goto failed;
    }

    if (!(p->features & IORING_FEAT_SINGLE_MMAP)) {
        if (munmap(u->sq_ptr, u->cq_len)) {
            goto failed;
        }
    }

    close(u->fd);
    return 0;

failed:
    close(u->fd);
    return -1;
}


int
caio_io_uring_submit(struct caio_io_uring *u, struct caio_task *task,
        struct io_uring_sqe *sqe) {

    return -1;
}


int
caio_io_uring_wait(struct caio_io_uring *u, int timeout) {
    return -1;
}


// /*
// * Read from completion queue.
// * In this function, we read completion events from the completion queue.
// * We dequeue the CQE, update and head and return the result of the operation.
// * */
// int read_from_cq() {
//     struct io_uring_cqe *cqe;
//     unsigned head;
//
//     /* Read barrier */
//     head = io_uring_smp_load_acquire(cring_head);
//     /*
//     * Remember, this is a ring buffer. If head == tail, it means that the
//     * buffer is empty.
//     * */
//     if (head == *cring_tail)
//         return -1;
//
//     /* Get the entry */
//     cqe = &cqes[head & (*cring_mask)];
//     if (cqe->res < 0)
//         fprintf(stderr, "Error: %s\n", strerror(abs(cqe->res)));
//
//     head++;
//
//     /* Write barrier so that update to the head are made visible */
//     io_uring_smp_store_release(cring_head, head);
//
//     return cqe->res;
// }
//
// /*
// * Submit a read or a write request to the submission queue.
// * */
//
// int submit_to_sq(int fd, int op) {
//     unsigned index, tail;
//
//     /* Add our submission queue entry to the tail of the SQE ring buffer */
//     tail = *sring_tail;
//     index = tail & *sring_mask;
//     struct io_uring_sqe *sqe = &sqes[index];
//     /* Fill in the parameters required for the read or write operation */
//     sqe->opcode = op;
//     sqe->fd = fd;
//     sqe->addr = (unsigned long) buff;
//     if (op == IORING_OP_READ) {
//         memset(buff, 0, sizeof(buff));
//         sqe->len = BLOCK_SZ;
//     }
//     else {
//         sqe->len = strlen(buff);
//     }
//     sqe->off = offset;
//
//     sring_array[index] = index;
//     tail++;
//
//     /* Update the tail */
//     io_uring_smp_store_release(sring_tail, tail);
//
//     /*
//     * Tell the kernel we have submitted events with the io_uring_enter()
//     * system call. We also pass in the IOURING_ENTER_GETEVENTS flag which
//     * causes the io_uring_enter() call to wait until min_complete
//     * (the 3rd param) events complete.
//     * */
//     int ret =  io_uring_enter(ring_fd, 1, 1, IORING_ENTER_GETEVENTS);
//     if (ret < 0) {
//         perror("io_uring_enter");
//         return -1;
//     }
//
//     return ret;
// }
//
// int main(int argc, char *argv[]) {
//     int res;
//
//     /* Setup io_uring for use */
//     if (app_setup_uring()) {
//         fprintf(stderr, "Unable to setup uring!\n");
//         return 1;
//     }
//
//     /*
//     * A while loop that reads from stdin and writes to stdout.
//     * Breaks on EOF.
//     */
//     while (1) {
//         /* Initiate read from stdin and wait for it to complete */
//         submit_to_sq(STDIN_FILENO, IORING_OP_READ);
//         /* Read completion queue entry */
//         res = read_from_cq();
//         if (res > 0) {
//             /* Read successful. Write to stdout. */
//             submit_to_sq(STDOUT_FILENO, IORING_OP_WRITE);
//             read_from_cq();
//         } else if (res == 0) {
//             /* reached EOF */
//             break;
//         }
//         else if (res < 0) {
//             /* Error reading file */
//             fprintf(stderr, "Error: %s\n", strerror(abs(res)));
//             break;
//         }
//         offset += res;
//     }
//
//     return 0;
// }
