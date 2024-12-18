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


#include "caio/caio.h"


#ifdef CONFIG_CAIO_ESP32
/* using sleep amount in miliseconds as state */
typedef unsigned long caio_sleep_t;
#else
/* using file descriptor as state */
typedef int caio_sleep_t;
#endif


#include "caio/fdmon.h"

#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY caio_sleep
#ifndef CONFIG_CAIO_ESP32
#define CAIO_ARG1 struct caio_fdmon *
#define CAIO_ARG2 time_t
#endif
#include "caio/generic.h"


#ifdef CONFIG_CAIO_ESP32

void
caio_esp32_sleep(struct caio_task *task, unsigned long us);

#define CAIO_SLEEP(task, us) \
    do { \
        (task)->current->line = __LINE__; \
        caio_esp32_sleep(task, us); \
        return; \
        case __LINE__:; \
    } while (0)

#else

#include <sys/timerfd.h>
#include "caio/fdmon.h"

int
caio_sleep_create(caio_sleep_t *sleep);

int
caio_sleep_destroy(caio_sleep_t *sleep);

ASYNC
caio_sleepA(struct caio_task *self, caio_sleep_t *state,
        struct caio_fdmon *iom, time_t miliseconds);

#define CAIO_SLEEP(self, state, iom, miliseconds) \
    CAIO_AWAIT(self, caio_sleep, caio_sleepA, state, \
            (struct caio_fdmon*)iom, miliseconds)

#endif  // CONFIG_CAIO_ESP32


#endif  // CAIO_SLEEP_H_
