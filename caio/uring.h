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
caio_uring_create(struct caio* c, unsigned int jobsmax, sigset_t *sigmask);


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


/* all-in-one functions */
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


#define caio_uring_submit(umod) io_uring_submit(&(u)->ring)
#define caio_uring_prep_read io_uring_prep_read
#define caio_uring_prep_write io_uring_prep_write
#define caio_uring_prep_readv io_uring_prep_readv
#define caio_uring_prep_writev io_uring_prep_writev
#define caio_uring_prep_socket io_uring_prep_socket
#define caio_uring_prep_accept io_uring_prep_accept
#define caio_uring_prep_accept_multishot io_uring_prep_multishot_accept
#define caio_uring_prep_accept_multishot_direct \
    io_uring_prep_multishot_accept_direct
#define caio_uring_prep_accept_direct io_uring_prep_accept_direct
#define caio_uring_prep_bind io_uring_prep_bind
#define caio_uring_prep_cancel io_uring_prep_cancel
#define caio_uring_prep_cancel64 io_uring_prep_cancel64
#define caio_uring_prep_cancel_fd io_uring_prep_cancel_fd
#define caio_uring_prep_close io_uring_prep_close
#define caio_uring_prep_close_direct io_uring_prep_close_direct
#define caio_uring_prep_cmd io_uring_prep_cmd
#define caio_uring_prep_connect io_uring_prep_connect
#define caio_uring_prep_fadvise io_uring_prep_fadvise
#define caio_uring_prep_fadvise64 io_uring_prep_fadvise64
#define caio_uring_prep_fallocate io_uring_prep_fallocate
#define caio_uring_prep_fgetxattr io_uring_prep_fgetxattr
#define caio_uring_prep_files_update io_uring_prep_files_update
#define caio_uring_prep_fixed_fd_install io_uring_prep_fixed_fd_install
#define caio_uring_prep_fsetxattr io_uring_prep_fsetxattr
#define caio_uring_prep_fsync io_uring_prep_fsync
#define caio_uring_prep_ftruncate io_uring_prep_ftruncate
#define caio_uring_prep_futex_wait io_uring_prep_futex_wait
#define caio_uring_prep_futex_waitv io_uring_prep_futex_waitv
#define caio_uring_prep_futex_wake io_uring_prep_futex_wake
#define caio_uring_prep_getxattr io_uring_prep_getxattr
#define caio_uring_prep_link io_uring_prep_link
#define caio_uring_prep_linkat io_uring_prep_linkat
#define caio_uring_prep_link_timeout io_uring_prep_link_timeout
#define caio_uring_prep_listen io_uring_prep_listen
#define caio_uring_prep_madvise io_uring_prep_madvise
#define caio_uring_prep_madvise64 io_uring_prep_madvise64
#define caio_uring_prep_mkdir io_uring_prep_mkdir
#define caio_uring_prep_mkdirat io_uring_prep_mkdirat
#define caio_uring_prep_msg_ring io_uring_prep_msg_ring
#define caio_uring_prep_msg_ring_cqe_flags io_uring_prep_msg_ring_cqe_flags
#define caio_uring_prep_msg_ring_fd io_uring_prep_msg_ring_fd
#define caio_uring_prep_msg_ring_fd_alloc io_uring_prep_msg_ring_fd_alloc
#define caio_uring_prep_nop io_uring_prep_nop
#define caio_uring_prep_openat io_uring_prep_openat
#define caio_uring_prep_openat2 io_uring_prep_openat2
#define caio_uring_prep_openat2_direct io_uring_prep_openat2_direct
#define caio_uring_prep_openat_direct io_uring_prep_openat_direct
#define caio_uring_prep_poll_add io_uring_prep_poll_add
#define caio_uring_prep_poll_multishot io_uring_prep_poll_multishot
#define caio_uring_prep_poll_remove io_uring_prep_poll_remove
#define caio_uring_prep_poll_update io_uring_prep_poll_update
#define caio_uring_prep_provide_buffers io_uring_prep_provide_buffers
#define caio_uring_prep_read_fixed io_uring_prep_read_fixed
#define caio_uring_prep_read_multishot io_uring_prep_read_multishot
#define caio_uring_prep_readv2 io_uring_prep_readv2
#define caio_uring_prep_recv io_uring_prep_recv
#define caio_uring_prep_recvmsg io_uring_prep_recvmsg
#define caio_uring_prep_recvmsg_multishot io_uring_prep_recvmsg_multishot
#define caio_uring_prep_recv_multishot io_uring_prep_recv_multishot
#define caio_uring_prep_remove_buffers io_uring_prep_remove_buffers
#define caio_uring_prep_rename io_uring_prep_rename
#define caio_uring_prep_renameat io_uring_prep_renameat
#define caio_uring_prep_send io_uring_prep_send
#define caio_uring_prep_send_bundle io_uring_prep_send_bundle
#define caio_uring_prep_sendmsg io_uring_prep_sendmsg
#define caio_uring_prep_sendmsg_zc io_uring_prep_sendmsg_zc
#define caio_uring_prep_send_set_addr io_uring_prep_send_set_addr
#define caio_uring_prep_sendto io_uring_prep_sendto
#define caio_uring_prep_send_zc io_uring_prep_send_zc
#define caio_uring_prep_send_zc_fixed io_uring_prep_send_zc_fixed
#define caio_uring_prep_setxattr io_uring_prep_setxattr
#define caio_uring_prep_shutdown io_uring_prep_shutdown
#define caio_uring_prep_socket_direct io_uring_prep_socket_direct
#define caio_uring_prep_socket_direct_alloc io_uring_prep_socket_direct_alloc
#define caio_uring_prep_splice io_uring_prep_splice
#define caio_uring_prep_statx io_uring_prep_statx
#define caio_uring_prep_symlink io_uring_prep_symlink
#define caio_uring_prep_symlinkat io_uring_prep_symlinkat
#define caio_uring_prep_sync_file_range io_uring_prep_sync_file_range
#define caio_uring_prep_tee io_uring_prep_tee
#define caio_uring_prep_timeout io_uring_prep_timeout
#define caio_uring_prep_timeout_remove io_uring_prep_timeout_remove
#define caio_uring_prep_timeout_update io_uring_prep_timeout_update
#define caio_uring_prep_unlink io_uring_prep_unlink
#define caio_uring_prep_unlinkat io_uring_prep_unlinkat
#define caio_uring_prep_waitid io_uring_prep_waitid
#define caio_uring_prep_write_fixed io_uring_prep_write_fixed
#define caio_uring_prep_writev2 io_uring_prep_writev2


#endif  // CAIO_URING_H_
