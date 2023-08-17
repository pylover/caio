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


/* Common stuff */
#define GARR_ISFULL(self) (self->count == self->size)
#define GARR_ISEMPTY(self) (self->count == 0)


/* Generic stuff */
#define GARRNAME_PASTER(x, y) x ## _ ## y
#define GARRNAME_EVALUATOR(x, y)  GARRNAME_PASTER(x, y)
#define GARRNAME(n) GARRNAME_EVALUATOR(GARR_TYPE, n)


struct
GARRNAME(array) {
    GARR_TYPE **array;
    size_t size;
    size_t count;
};


int
GARRNAME(array_init)(struct GARRNAME(array) *self, size_t size);


int
GARRNAME(array_append)(struct GARRNAME(array) *self, GARR_TYPE *item);


int
GARRNAME(array_set)(struct GARRNAME(array) *self, GARR_TYPE *item,
        unsigned int index);


GARR_TYPE*
GARRNAME(array_get)(struct GARRNAME(array) *self, unsigned int index);
