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
#define CAIO_ARG1 time_t
#include "caio/generic.h"


int
caio_sleep_create(caio_sleep_t *sleep);


int
caio_sleep_destroy(caio_sleep_t *sleep);


#ifdef CAIO_SELECT

#include "caio/select.h"

#define CAIO_SLEEP_SELECT(select, self, sleep, seconds) \
    CAIO_AWAIT(self, caio_sleep, caio_sleepA, seconds, select)

#endif

/*
#define CAIO_SLEEP(self, state, ...) \
    CAIO_AWAIT(self, sleep, caio_sleepA, state, __VA_ARGS__)
*/
//
//
// ASYNC
// caio_sleepA(struct caio_task *self, int *state, time_t seconds);


#endif  // CAIO_SLEEP_H_
