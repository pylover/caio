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
 *
 *
 * An edge-triggered epoll(7) example using caio.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#include <liburing.h>

#include "caio/config.h"
#include "caio/caio.h"


#define MAXTASKS 1
#define CHUNKSIZE 1024


struct fileinfo {
    int fd;
    off_t size;
    int blocks;
    struct iovec iovecs[];
};


static struct caio *_caio;
static struct sigaction oldaction;


typedef struct cat {
    int argc;
    const char **argv;
    struct io_uring uring;
} cat_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY cat
#include "caio/generic.h"
#include "caio/generic.c"


static void
_sighandler(int s) {
    printf("\nsignal: %d\n", s);
    caio_task_killall(_caio);
    printf("\n");
}


static int
_handlesignals() {
    struct sigaction new_action = {{_sighandler}, {{0, 0, 0, 0}}};
    if (sigaction(SIGINT, &new_action, &oldaction) != 0) {
        return -1;
    }

    return 0;
}


/*
* Returns the size of the file whose open file descriptor is passed in.
* Properly handles regular file and block devices as well. Pretty.
* */
static off_t
_filesize_get(int fd) {
    struct stat st;

    if(fstat(fd, &st) < 0) {
        perror("fstat");
        return -1;
    }

    if (S_ISBLK(st.st_mode)) {
        unsigned long long bytes;
        if (ioctl(fd, BLKGETSIZE64, &bytes) != 0) {
            perror("ioctl");
            return -1;
        }
        return bytes;
    } else if (S_ISREG(st.st_mode))
        return st.st_size;

    return -1;
}


static int
_fileinfo(const char *filename, struct fileinfo **infoptr) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    off_t filesize = _filesize_get(fd);
    off_t remaining = filesize;
    off_t offset = 0;
    int i = 0;
    int blocks = (int) filesize / CHUNKSIZE;
    if (filesize % CHUNKSIZE) {
        blocks++;
    }

    *infoptr = malloc(sizeof(struct fileinfo) +
            sizeof(struct iovec) * blocks);
    struct fileinfo *info = *infoptr;
    if (info == NULL) {
        return -1;
    }

    while (remaining) {
        off_t toread = remaining;
        if (toread > CHUNKSIZE) {
            toread = CHUNKSIZE;
        }

        offset += toread;
        info->iovecs[i].iov_len = toread;
        void *buf;
        if(posix_memalign(&buf, CHUNKSIZE, CHUNKSIZE)) {
            perror("posix_memalign");
            goto failed;
        }
        info->iovecs[i].iov_base = buf;

        i++;
        remaining -= toread;
    }
    info->size = filesize;
    info->blocks = blocks;
    info->fd = fd;
    return 0;

failed:
    free(*infoptr);
    while (--i >= 0) {
        free(info->iovecs[i].iov_base);
    }

    return -1;
}


/*
 * Submit the readv request via liburing
 * */
static int
_submit_readfile(struct fileinfo *info, struct io_uring *ring) {
    /* Get an SQE */
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);

    /* Setup a readv operation */
    io_uring_prep_readv(sqe, info->fd, info->iovecs, info->blocks, 0);

    /* Set user data */
    io_uring_sqe_set_data(sqe, info);

    /* Finally, submit the request */
    io_uring_submit(ring);

    return 0;
}


/*
 * Submit the writev request via liburing
 * */
static int
_submit_writefile(struct fileinfo *info, struct io_uring *ring) {
    /* Get an SQE */
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);

    /* Setup a readv operation */
    io_uring_prep_writev(sqe, info->fd, info->iovecs, info->blocks, 0);

    /* Set user data */
    io_uring_sqe_set_data(sqe, info);

    /* Finally, submit the request */
    io_uring_submit(ring);

    return 0;
}


static int
_get_readv_completion(struct fileinfo **info, struct io_uring *ring) {
    struct io_uring_cqe *cqe;
    if (io_uring_wait_cqe(ring, &cqe) < 0) {
        perror("io_uring_wait_cqe");
        return -1;
    }

    if (cqe->res < 0) {
        fprintf(stderr, "Async readv failed.\n");
        return -1;
    }

    *info = io_uring_cqe_get_data(cqe);
    io_uring_cqe_seen(ring, cqe);
    return 0;
}


static int
_get_writev_completion(struct fileinfo **info, struct io_uring *ring) {
    struct io_uring_cqe *cqe;
    if (io_uring_wait_cqe(ring, &cqe) < 0) {
        perror("io_uring_wait_cqe");
        return -1;
    }

    if (cqe->res < 0) {
        fprintf(stderr, "Async readv failed.\n");
        return -1;
    }

    *info = io_uring_cqe_get_data(cqe);
    io_uring_cqe_seen(ring, cqe);
    return 0;
}


static ASYNC
catA(struct caio_task *self, struct cat *state) {
    static int i;
    struct fileinfo *info;
    CAIO_BEGIN(self);

    for (i = 1; i < state->argc; i++) {
        if (_fileinfo(state->argv[i], &info)) {
            perror("create file info");
            CAIO_THROW(self, errno);
        }

        if (_submit_readfile(info, &state->uring)) {
            perror("submit readv");
            CAIO_THROW(self, errno);
        }

        // TODO: wait
        _get_readv_completion(&info, &state->uring);
        close(info->fd);
        info->fd = STDOUT_FILENO;
        if (_submit_writefile(info, &state->uring)) {
            perror("submit writev");
            CAIO_THROW(self, errno);
        }

        // TODO: wait
        _get_writev_completion(&info, &state->uring);
        fsync(STDOUT_FILENO);
        free(info);
    }

    CAIO_FINALLY(self);
}


int
main(int argc, const char **argv) {
    int exitstatus = EXIT_SUCCESS;
    struct cat state;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s FILENAME1 [FILENAME2...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    memset(&state, 0, sizeof(struct cat));
    state.argc = argc;
    state.argv = argv;

    if (_handlesignals()) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    _caio = caio_create(MAXTASKS);
    if (_caio == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    /* Initialize io_uring */
    if (io_uring_queue_init(MAXTASKS, &state.uring, 0) < 0) {
        perror("io_uring setup failed!");
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    cat_spawn(_caio, catA, &state);
    if (caio_loop(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

terminate:
    io_uring_queue_exit(&state.uring);

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    return exitstatus;
}
