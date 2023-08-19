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


#ifndef GENERIC_ARRAY_COMMON_H_
#define GENERIC_ARRAY_COMMON_H_
#define GARR_ISFULL(self) (self->count == self->size)
#define GARR_ISEMPTY(self) (self->count == 0)
#endif  // GENERIC_ARRAY_COMMON_H_


/* Generic stuff */
#define GARRNAME_PASTER(x, y, z) x ## y ## z
#define GARRNAME_EVALUATOR(x, y, z)  GARRNAME_PASTER(x, y, z)
#define GARRNAME(n) GARRNAME_EVALUATOR(GARR_TYPE, _array_, n)
#define GARRSELF() GARRNAME_EVALUATOR(GARR_TYPE, _, array)


struct
GARRSELF() {
    GARR_TYPE **array;
    size_t size;
    size_t count;
};


int
GARRNAME(init)(struct GARRSELF() *self, size_t size);


void
GARRNAME(deinit)(struct GARRSELF() *self);


int
GARRNAME(append)(struct GARRSELF() *self, GARR_TYPE *item);


int
GARRNAME(set)(struct GARRSELF() *self, GARR_TYPE *item,
        unsigned int index);


GARR_TYPE*
GARRNAME(get)(struct GARRSELF() *self, unsigned int index);


int
GARRNAME(del)(struct GARRSELF() *self, unsigned int index);
