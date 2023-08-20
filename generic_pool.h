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


#ifndef GENERIC_POOL_H_
#define GENERIC_POOL_H_
#define GPOOL_ISFULL(self) ((self)->count == (self)->size)
#define GPOOL_ISEMPTY(self) ((self)->count == 0)
#endif  // GENERIC_POOL_H_


/* Generic stuff */
#define GPOOLNAME_PASTER(x, y, z) x ## y ## z
#define GPOOLNAME_EVALUATOR(x, y, z)  GPOOLNAME_PASTER(x, y, z)
#define GPOOLNAME(n) GPOOLNAME_EVALUATOR(GPOOLTYPE, _pool_, n)
#define GPOOLSELF() GPOOLNAME_EVALUATOR(GPOOLTYPE, _, pool)


struct
GPOOLSELF() {
    GPOOLTYPE **pool;
    size_t size;
    size_t count;
};


typedef void (*GPOOLNAME(vacuumcb)) (GPOOLTYPE *item, unsigned int index);


int
GPOOLNAME(init)(struct GPOOLSELF() *self, size_t size);


void
GPOOLNAME(deinit)(struct GPOOLSELF() *self);


int
GPOOLNAME(append)(struct GPOOLSELF() *self, GPOOLTYPE *item);


int
GPOOLNAME(vacuumflag)(struct GPOOLSELF() *self, unsigned int index);


GPOOLTYPE*
GPOOLNAME(get)(struct GPOOLSELF() *self, unsigned int index);


void
GPOOLNAME(vacuum)(struct GPOOLSELF() *self, GPOOLNAME(vacuumcb) cb);
