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
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>

#include <liburing.h>

#include "caio/config.h"
#include "caio/caio.h"


#define QUEUE_DEPTH 1
#define BLOCKSIZE 1024


static struct caio *_caio;
static struct sigaction oldaction;


struct file_info {
    off_t filesize;
    long blocks;
    struct iovec *iovecs;
};


typedef struct cat {
    struct io_uring ring;
    const char **argv;
    int argc;
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
_get_file_size(int fd) {
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


/*
 * Submit the readv request via liburing
 * */
static int
_submit_read_request(struct io_uring *ring, const char *file_path) {
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        perror("open");
        return 1;
    }
    off_t filesize = _get_file_size(file_fd);
    off_t bytes_remaining = filesize;
    off_t offset = 0;
    int current_block = 0;
    int blocks = (int) filesize / BLOCKSIZE;
    if (filesize % BLOCKSIZE) blocks++;
    struct file_info *fi = malloc(sizeof(struct file_info));
    printf("alloc fi: %p %p\n", fi, &fi->iovecs);
    if (fi == NULL) {
        perror("malloc");
        return -1;
    }
    fi->iovecs = calloc(blocks, sizeof(struct iovec));
    printf("alloc iovecs: %p\n", fi->iovecs);
    if (fi->iovecs == NULL) {
        perror("calloc");
        return -1;
    }
    fi->blocks = blocks;

    /*
     * For each block of the file we need to read, we allocate an iovec struct
     * which is indexed into the iovecs array. This array is passed in as part
     * of the submission. If you don't understand this, then you need to look
     * up how the readv() and writev() system calls work.
     * */
    while (bytes_remaining) {
        off_t bytes_to_read = bytes_remaining;
        if (bytes_to_read > BLOCKSIZE)
            bytes_to_read = BLOCKSIZE;

        offset += bytes_to_read;
        fi->iovecs[current_block].iov_len = bytes_to_read;
        void *buf = NULL;
        if(posix_memalign(&buf, BLOCKSIZE, BLOCKSIZE)) {
            perror("posix_memalign");
            return 1;
        }
        printf("posix alloc: %p\n", buf);
        fi->iovecs[current_block].iov_base = buf;

        current_block++;
        bytes_remaining -= bytes_to_read;
    }
    fi->filesize = filesize;

    /* Get an SQE */
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);

    /* Setup a readv operation */
    io_uring_prep_readv(sqe, file_fd, fi->iovecs, blocks, 0);

    /* Set user data */
    io_uring_sqe_set_data(sqe, fi);

    /* Finally, submit the request */
    io_uring_submit(ring);

    return 0;
}


/*
 * Output a string of characters of len length to stdout.
 * We use buffered output here to be efficient,
 * since we need to output character-by-character.
 * */
static void
output_to_console(const char *buf, int len) {
    printf("out buf: %p\n", buf);
    while (len--) {
        fputc(*buf++, stdout);
    }
}


/*
 * Wait for a completion to be available, fetch the data from
 * the readv operation and print it to the console.
 * */
static int
_get_completion_and_print(struct io_uring *ring) {
    int i;
    struct io_uring_cqe *cqe;

    int ret = io_uring_wait_cqe(ring, &cqe);
    if (ret < 0) {
        perror("io_uring_wait_cqe");
        return 1;
    }
    if (cqe->res < 0) {
        perror("Async readv failed.");
        return 1;
    }
    struct file_info *fi = io_uring_cqe_get_data(cqe);
    printf("retrv fi: %p\n", fi);
    printf("vect: %p\n", fi->iovecs);
    for (i = 0; i < fi->blocks; i++) {
        printf("block: %p\n", &fi->iovecs[i]);
        output_to_console(fi->iovecs[i].iov_base, fi->iovecs[i].iov_len);
    }

    io_uring_cqe_seen(ring, cqe);

    for (i = 0; i < fi->blocks; i++) {
        free(fi->iovecs[i].iov_base);
    }
    free(fi->iovecs);
    free(fi);
    return 0;
}


static ASYNC
catA(struct caio_task *self, struct cat *state) {
    int i;
    CAIO_BEGIN(self);

    for (i = 0; i < state->argc; i++) {
        int ret = _submit_read_request(&state->ring, state->argv[i]);
        if (ret) {
            fprintf(stderr, "Error reading file: %s\n", state->argv[i]);
            CAIO_THROW(self, errno);
        }
        _get_completion_and_print(&state->ring);
    }

    CAIO_FINALLY(self);
}


int
main(int argc, const char **argv) {
    int exitstatus = EXIT_SUCCESS;
    struct cat state;
    memset(&state, 0, sizeof(struct cat));

    state.argv = argv + 1;
    state.argc = argc - 1;

    if (_handlesignals()) {
        return EXIT_FAILURE;
    }

    _caio = caio_create(QUEUE_DEPTH);
    if (_caio == NULL) {
        return EXIT_FAILURE;
    }

    /* Initialize io_uring */
    if (io_uring_queue_init(QUEUE_DEPTH, &state.ring, 0) != 0) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    cat_spawn(_caio, catA, &state);
    if (caio_loop(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

terminate:
    io_uring_queue_exit(&state.ring);

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    return exitstatus;
}
