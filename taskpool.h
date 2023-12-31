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
#ifndef TASKPOOL_H_
#define TASKPOOL_H_


#include <stddef.h>
#include <stdbool.h>

#include "caio.h"


#define TASKPOOL_ISFULL(self) ((self)->count == (self)->size)
#define TASKPOOL_ISEMPTY(self) ((self)->count == 0)


int
taskpool_init(struct caio_taskpool *self, size_t size);


void
taskpool_deinit(struct caio_taskpool *self);


int
taskpool_append(struct caio_taskpool *self, struct caio_task *item);


int
taskpool_delete(struct caio_taskpool *self, unsigned int index);


struct caio_task*
taskpool_get(struct caio_taskpool *self, unsigned int index);


void
taskpool_vacuum(struct caio_taskpool *self);


#endif  // TASKPOOL_H_
