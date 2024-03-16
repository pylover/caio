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


#define CAIO_RING_AVAIL(r) (((*(r).head - *(r).tail - 1) & *(r).mask))
#define CAIO_RING_USED(r) ((*(r).tail - *(r).head) & *(r).mask)
#define CAIO_RING_ISFULL(r) (CAIO_RING_USED(r) == *(r).mask)


struct caio_io_uring_mapinfo {
    void *start;
    size_t size;

    unsigned int *tail;
    unsigned int *head;
    unsigned int *mask;
};


struct caio_io_uring_sq {
    struct caio_io_uring_mapinfo;
    struct io_uring_sqe *array;
};


struct caio_io_uring_cq {
    struct caio_io_uring_mapinfo;
    struct io_uring_cqe *array;
};


struct caio_io_uring {
    int fd;
    struct io_uring_params params;
    struct caio_io_uring_sq sq;
    struct caio_io_uring_cq cq;
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
 *
 * struct io_uring_sqe {
 *     __u8    opcode;     // type of operation for this sqe
 *     __u8    flags;      // IOSQE_ flags
 *     __u16   ioprio;     // ioprio for the request
 *     __s32   fd;         // file descriptor to do IO on
 *     union {
 *         __u64   off;    // offset into file
 *         __u64   addr2;
 *     };
 *     union {
 *         __u64   addr;   // pointer to buffer or iovecs
 *         __u64   splice_off_in;
 *     };
 *     __u32   len;        // buffer size or number of iovecs
 *     union {
 *         __kernel_rwf_t  rw_flags;
 *         __u32       fsync_flags;
 *         __u16       poll_events;    // compatibility
 *         __u32       poll32_events;  // word‐reversed for BE
 *         __u32       sync_range_flags;
 *         __u32       msg_flags;
 *         __u32       timeout_flags;
 *         __u32       accept_flags;
 *         __u32       cancel_flags;
 *         __u32       open_flags;
 *         __u32       statx_flags;
 *         __u32       fadvise_advice;
 *         __u32       splice_flags;
 *     };
 *     __u64   user_data;      // data to be passed back at completion time
 *     union {
 *         struct {
 *             // pack this to avoid bogus arm OABI complaints
 *             union {
 *                 // index into fixed buffers, if used
 *                 __u16   buf_index;
 *                 // for grouped buffer selection
 *                 __u16   buf_group;
 *             } __attribute__((packed));
 *             // personality to use, if used
 *             __u16   personality;
 *             __s32   splice_fd_in;
 *         };
 *         __u64   __pad2[3];
 *     };
 * };
 */


int
caio_io_uring_init(struct caio_io_uring *u, size_t maxtasks);


int
caio_io_uring_deinit(struct caio_io_uring *u);


int
caio_io_uring_wait(struct caio_io_uring *u, int timeout);


#endif  // CAIO_IO_URING_H_
