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
#ifndef SLEEP_H_
#define SLEEP_H_


#include "caio.h"


typedef int sleep_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY sleep
#define CAIO_ARG1 time_t
#include "generic.h"


#define CORO_SLEEP(self, state, ...) \
    AWAIT(self, sleep, caio_sleepA, state, __VA_ARGS__)


ASYNC
caio_sleepA(struct caio_task *self, int *state, time_t seconds);


#endif  // SLEEP_H_
