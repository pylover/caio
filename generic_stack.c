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
#include <stdlib.h>  // NOLINT
#include <string.h>


int
GSTACKNAME(init)(struct GSTACKSELF() *self, size_t size) {
    self->stack = calloc(size, sizeof(GSTACK_TYPE*));
    if (self->stack == NULL) {
        return -1;
    }
    memset(self->stack, 0, size * sizeof(GSTACK_TYPE*));
    self->count = 0;
    self->size = size;
    return 0;
}


void
GSTACKNAME(deinit)(struct GSTACKSELF() *self) {
    if (self->stack == NULL) {
        return;
    }
    free(self->stack);
}


int
GSTACKNAME(push)(struct GSTACKSELF() *self, GSTACK_TYPE *item) {
    int index;

    if (item == NULL) {
        return -1;
    }

    if (GSTACK_ISFULL(self)) {
        return -1;
    }

    index = self->count++;
    self->stack[index] = item;
    return index;
}


GSTACK_TYPE*
GSTACKNAME(pop)(struct GSTACKSELF() *self) {
    if (self->count <= 0) {
        return NULL;
    }

    return self->stack[--self->count];
}


GSTACK_TYPE*
GSTACKNAME(last)(struct GSTACKSELF() *self) {
    if (self->count <= 0) {
        return NULL;
    }

    return self->stack[self->count - 1];
}
