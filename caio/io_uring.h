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
#ifndef CAIO_IO_URING_H_
#define CAIO_IO_URING_H_


#include <linux/io_uring.h>


struct caio_io_uring {
    int ringfd;
    struct io_uring_params params;

    struct io_uring_sqe *sqes;
    struct io_uring_cqe *cqes;
    unsigned int *sq_tail;
    unsigned int *sq_mask;
    unsigned int *sq_array;

    unsigned int *cq_head;
    unsigned int *cq_tail;
    unsigned int *cq_mask;
};


/*
 * io_uring structures
 *
 * struct io_uring_params {
 *     __u32 sq_entries;
 *     __u32 cq_entries;
 *     __u32 flags;
 *     __u32 sq_thread_cpu;
 *     __u32 sq_thread_idle;
 *     __u32 features;
 *     __u32 wq_fd;
 *     __u32 resv[3];
 *     struct io_sqring_offsets sq_off;
 *     struct io_cqring_offsets cq_off;
 * };
 *
 * struct io_sqring_offsets {
 *     __u32 head;
 *     __u32 tail;
 *     __u32 ring_mask;
 *     __u32 ring_entries;
 *     __u32 flags;
 *     __u32 dropped;
 *     __u32 array;
 *     __u32 resv[3];
 * };
 *
 * struct io_cqring_offsets {
 *     __u32 head;
 *     __u32 tail;
 *     __u32 ring_mask;
 *     __u32 ring_entries;
 *     __u32 overflow;
 *     __u32 cqes;
 *     __u32 flags;
 *     __u32 resv[3];
 * };
 *
 */
int
caio_io_uring_init(struct caio_io_uring *u, size_t maxtasks);


void
caio_io_uring_deinit(struct caio_io_uring *u);


#endif  // CAIO_IO_URING_H_
