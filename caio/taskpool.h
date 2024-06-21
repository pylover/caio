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
#ifndef CAIO_TASKPOOL_H_
#define CAIO_TASKPOOL_H_


#include <stddef.h>
#include <stdbool.h>

#include "caio.h"


struct caio_taskpool {
    struct caio_task *tasks;
    struct caio_task *last;
    size_t size;
    size_t count;
};


int
caio_taskpool_init(struct caio_taskpool *p, size_t size);


int
caio_taskpool_deinit(struct caio_taskpool *pool);


struct caio_task *
caio_taskpool_next(struct caio_taskpool *pool, struct caio_task *task,
        enum caio_taskstatus statuses);


struct caio_task *
caio_taskpool_lease(struct caio_taskpool *pool);


int
caio_taskpool_release(struct caio_taskpool *pool, struct caio_task *task);


#endif  // CAIO_TASKPOOL_H_
