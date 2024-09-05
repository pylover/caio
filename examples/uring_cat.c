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
 * An io_uring(7) example of cat
 */
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include "caio/config.h"
#include "caio/caio.h"
#include "caio/uring.h"


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
    struct caio_uring *uring;
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

    if (fstat(fd, &st) < 0) {
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


void
_fileinfo_dispose(struct fileinfo *info) {
    if (info == NULL) {
        return;
    }

    int i = info->blocks;
    while (--i >= 0) {
        free(info->iovecs[i].iov_base);
    }

    free(info);
}


static struct fileinfo *
_fileinfo_create(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return NULL;
    }
    off_t filesize = _filesize_get(fd);
    off_t remaining = filesize;
    off_t offset = 0;
    int i = 0;
    int blocks = (int) filesize / CHUNKSIZE;
    if (filesize % CHUNKSIZE) {
        blocks++;
    }

    struct fileinfo *info = malloc(sizeof(struct fileinfo) +
            sizeof(struct iovec) * blocks);
    if (info == NULL) {
        perror("out of memory");
        return NULL;
    }

    while (remaining) {
        off_t toread = remaining;
        if (toread > CHUNKSIZE) {
            toread = CHUNKSIZE;
        }

        offset += toread;
        info->iovecs[i].iov_len = toread;
        void *buf;
        if (posix_memalign(&buf, CHUNKSIZE, CHUNKSIZE)) {
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
    return info;

failed:
    _fileinfo_dispose(info);

    return NULL;
}


static ASYNC
catA(struct caio_task *self, struct cat *state) {
    int ret;
    static int i;
    static struct fileinfo *info;
    CAIO_BEGIN(self);

    for (i = 1; i < state->argc; i++) {
        /* open, get filesize and allocate buffer */
        info = _fileinfo_create(state->argv[i]);
        if (info == NULL) {
            perror("create file info");
            CAIO_THROW(self, errno);
        }

        /* create, setup and submit a sqe for read the file into buffer
         */
        ret = caio_uring_readv(
                state->uring,
                self,
                info->fd,
                info->iovecs,
                info->blocks,
                0);
        if (ret < 0) {
            perror("io_uring readv submit.");
            CAIO_THROW(self, -ret);
        }

        /* wait for task to complete */
        CAIO_URING_AWAIT(state->uring, self, 1);
        caio_uring_cqe_seen(state->uring, self, 0);

        /* close the file descriptor. */
        close(info->fd);

        /* create, setup and submit a sqe to write into stdout from the buffer
         */
        ret = caio_uring_writev(
                state->uring,
                self,
                STDOUT_FILENO,
                info->iovecs,
                info->blocks,
                0);
        if (ret < 0) {
            perror("io_uring task submit.");
            CAIO_THROW(self, -ret);
        }

        /* wait for task to complete */
        CAIO_URING_AWAIT(state->uring, self, 1);
        caio_uring_cqe_seen(state->uring, self, 0);

        /* cleanup */
        fsync(STDOUT_FILENO);
        _fileinfo_dispose(info);
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
    state.uring = caio_uring_create(_caio, MAXTASKS, 0, NULL);
    if (state.uring == NULL) {
        perror("io_uring setup failed!");
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    cat_spawn(_caio, catA, &state);
    if (caio_loop(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

terminate:
    caio_uring_destroy(_caio, state.uring);

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    return exitstatus;
}
