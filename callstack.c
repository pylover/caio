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
#include <stdlib.h>
#include <string.h>

#include "callstack.h"


int
caio_callstack_init(struct caio_callstack *self, size_t size) {
    self->stack = calloc(size, sizeof(struct caio_call*));
    if (self->stack == NULL) {
        return -1;
    }
    memset(self->stack, 0, size * sizeof(struct caio_call*));
    self->count = 0;
    self->size = size;
    return 0;
}


void
caio_callstack_deinit(struct caio_callstack *self) {
    if (self->stack == NULL) {
        return;
    }
    free(self->stack);
}


int
caio_callstack_push(struct caio_callstack *self, struct caio_call *item) {
    int index;

    if (item == NULL) {
        return -1;
    }

    if (CALLSTACK_ISFULL(self)) {
        return -1;
    }

    index = self->count++;
    self->stack[index] = item;
    return index;
}


struct caio_call*
caio_callstack_pop(struct caio_callstack *self) {
    if (self->count <= 0) {
        return NULL;
    }

    return self->stack[--self->count];
}


struct caio_call*
caio_callstack_last(struct caio_callstack *self) {
    if (self->count <= 0) {
        return NULL;
    }

    return self->stack[self->count - 1];
}
