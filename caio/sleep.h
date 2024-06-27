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
#ifndef CAIO_SLEEP_H_
#define CAIO_SLEEP_H_


#include <sys/timerfd.h>
#include "caio/caio.h"


typedef int caio_sleep_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY caio_sleep
#define CAIO_ARG1 struct caio_iomodule *
#define CAIO_ARG2 time_t
#include "caio/generic.h"


int
caio_sleep_create(caio_sleep_t *sleep);


int
caio_sleep_destroy(caio_sleep_t *sleep);


ASYNC
caio_sleepA(struct caio_task *self, caio_sleep_t *state,
        struct caio_iomodule *iom, time_t miliseconds);


#endif  // CAIO_SLEEP_H_
