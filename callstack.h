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
#ifndef CALLSTACK_H_
#define CALLSTACK_H_


#include <stddef.h>
#include <stdbool.h>

#include "caio.h"


#define CALLSTACK_ISFULL(self) (self->count == self->size)
#define CALLSTACK_ISEMPTY(self) (self->count == 0)


int
caio_callstack_init(struct caio_callstack *self, size_t size);


void
caio_callstack_deinit(struct caio_callstack *self);


int
caio_callstack_push(struct caio_callstack *self, struct caio_call *item);


struct caio_call*
caio_callstack_pop(struct caio_callstack *self);


struct caio_call*
caio_callstack_last(struct caio_callstack *self);


#endif  // CALLSTACK_H_
