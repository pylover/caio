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
#ifndef CAIO_SELECT_H_
#define CAIO_SELECT_H_


#include <sys/select.h>

#include <caio/caio.h>


typedef struct caio_select *caio_select_t;


struct caio_select *
caio_select_create(caio_t c, unsigned int timeout);


int
caio_select_destroy(caio_t c, caio_select_t s);


int
caio_select_monitor(caio_select_t s, struct caio_task *task, int fd,
        int events);


int
caio_select_forget(struct caio_select *s, int fd);


#define CAIO_SELECT_WAIT(s, task, fd, events) \
    do { \
        (task)->current->line = __LINE__; \
        if (caio_select_monitor(s, task, fd, events)) { \
            (task)->status = CAIO_TERMINATING; \
        } \
        else { \
            (task)->status = CAIO_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#endif  // CAIO_SELECT_H_
