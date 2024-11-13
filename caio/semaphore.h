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
#ifndef CAIO_SEMAPHORE_H_
#define CAIO_SEMAPHORE_H_


#include "caio/caio.h"


typedef struct caio_semaphore {
    volatile int value;
    struct caio_task *task;
} caio_semaphore_t;


int
caio_semaphore_begin(struct caio_task *task, struct caio_semaphore *s);


int
caio_semaphore_end(struct caio_task *task);


int
caio_semaphore_acquire(struct caio_task *task);


int
caio_semaphore_release(struct caio_task *task);


#endif  // CAIO_SEMAPHORE_H_
