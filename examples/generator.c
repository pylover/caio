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
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include <clog.h>

#include "caio/config.h"
#include "caio/caio.h"


typedef struct generator {
    const char *name;
    int count;
} generator_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY generator
#define CAIO_ARG1 int*
#include "caio/generic.h"
#include "caio/generic.c"


typedef void consumer_t;
#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY consumer
#include "caio/generic.h"  // NOLINT
#include "caio/generic.c"  // NOLINT


static ASYNC
producerA(struct caio_task *self, struct generator *state, int *out) {
    CAIO_BEGIN(self);
    if (state->count >= 5) {
        CAIO_THROW(self, ECANCELED);
    }
    *out = state->count++;
    CAIO_FINALLY(self);
}


static ASYNC
consumerA(struct caio_task *self, consumer_t *_) {
    static struct generator foo = {.name = "foo", 0};
    static struct generator bar = {.name = "bar", 0};
    static int value;
    CAIO_BEGIN(self);
    while (true) {
        CAIO_AWAIT(self, generator, producerA, &foo, &value);
        if (!CAIO_HASERROR(self)) {
            INFO("foo yields: %d", value);
        }

        CAIO_AWAIT(self, generator, producerA, &bar, &value);
        if (!CAIO_HASERROR(self)) {
            INFO("bar yields: %d", value);
        }
        else if (CAIO_ISERROR(self, ECANCELED)) {
            INFO("bar stopped");
            CAIO_CLEARERROR(self);
            break;
        }
        else {
            ERROR("Unknowd error: %d", self->eno);
            break;
        }
    }
    INFO("foo called %d times.", foo.count);
    INFO("bar called %d times.", bar.count);
    CAIO_FINALLY(self);
}


int
main() {
    return consumer_forever(consumerA, NULL, 1);
}
