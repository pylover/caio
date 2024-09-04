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


#include "caio/caio.h"


struct caio_fdmon;
typedef int (*caio_filemonitor) (struct caio_fdmon *iom,
        struct caio_task *task, int fd, int events);
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
        if ((fdmon)->monitor(fdmon, task, fd, events)) { \
            (task)->status = CAIO_TERMINATING; \
        } \
        else { \
            (task)->status = CAIO_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#endif  // CAIO_FDMON_H_
