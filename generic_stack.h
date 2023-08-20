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
#include <stddef.h>
#include <stdbool.h>


#ifndef GENERIC_STACK_H_
#define GENERIC_STACK_H_
#define GSTACK_ISFULL(self) (self->count == self->size)
#define GSTACK_ISEMPTY(self) (self->count == 0)
#endif  // GENERIC_STACK_H_


/* Generic stuff */
#define GSTACKNAME_PASTER(x, y, z) x ## y ## z
#define GSTACKNAME_EVALUATOR(x, y, z)  GSTACKNAME_PASTER(x, y, z)
#define GSTACKNAME(n) GSTACKNAME_EVALUATOR(GSTACK_TYPE, _stack_, n)
#define GSTACKSELF() GSTACKNAME_EVALUATOR(GSTACK_TYPE, _, stack)


struct
GSTACKSELF() {
    GSTACK_TYPE **stack;
    size_t size;
    size_t count;
};


int
GSTACKNAME(init)(struct GSTACKSELF() *self, size_t size);


void
GSTACKNAME(deinit)(struct GSTACKSELF() *self);


int
GSTACKNAME(push)(struct GSTACKSELF() *self, GSTACK_TYPE *item);


GSTACK_TYPE*
GSTACKNAME(pop)(struct GSTACKSELF() *self);


GSTACK_TYPE*
GSTACKNAME(last)(struct GSTACKSELF() *self);
