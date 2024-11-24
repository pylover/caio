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
#ifndef CAIO_FDMON_H_
#define CAIO_FDMON_H_


#include <time.h>

#include "caio/caio.h"


struct caio_fdmon;
typedef int (*caio_filemonitor) (struct caio_fdmon *iom,
        struct caio_task *task, int fd, int events, unsigned int timeout_us);
typedef int (*caio_fileforget) (struct caio_fdmon *iom, int fd);
struct caio_fdmon {
    struct caio_module;
    caio_filemonitor monitor;
    caio_fileforget forget;
};


#define CAIO_FILE_FORGET(fdmon, fd) (fdmon)->forget(fdmon, fd)
#define CAIO_FILE_AWAIT(fdmon, task, fd, events) \
    do { \
        (task)->current->line = __LINE__; \
        if ((fdmon)->monitor(fdmon, task, fd, events, 0)) { \
            (task)->status = CAIO_TERMINATING; \
        } \
        else { \
            (task)->status = CAIO_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#define CAIO_FILE_TIMEDOUT(task) ((task)->fdmon_timeout_us < 0)
#define CAIO_FILE_TWAIT(fdmon, task, fd, events, us) \
    do { \
        (task)->current->line = __LINE__; \
        if ((fdmon)->monitor(fdmon, task, fd, events, us)) { \
            (task)->status = CAIO_TERMINATING; \
        } \
        else { \
            (task)->status = CAIO_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


/* IO helper macros */
#define CAIO_IN 0x1
#define CAIO_ERR 0x2
#define CAIO_OUT 0x4
#define CAIO_MUSTWAIT(e) \
    (((e) == EAGAIN) || ((e) == EWOULDBLOCK) || ((e) == EINPROGRESS))


long
timediff(struct timespec start, struct timespec end);


void
fdmon_tasks_timeout_check(struct caio *c);


long
fdmon_task_timeout_us(struct caio_task *task);


#endif  // CAIO_FDMON_H_
