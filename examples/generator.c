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
#include "caio.h"


typedef struct generator {
    const char *name;
    int count;
} generator_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY generator
#define CAIO_ARG1 int*
#include "generic.h"
#include "generic.c"


static ASYNC
producerA(struct caio_task *self, struct generator *state, int *out) {
    CORO_START;
    if (state->count >= 5) {
        CORO_REJECT(ECANCELED);
    }
    *out = state->count++;
    CORO_FINALLY;
}


static ASYNC
consumerA(struct caio_task *self) {
    static struct generator foo = {.name = "foo", 0};
    static struct generator bar = {.name = "bar", 0};
    static int value;
    CORO_START;
    while (true) {
        AWAIT(generator, producerA, &foo, &value);
        INFO("foo yields: %d", value);

        AWAIT(generator, producerA, &bar, &value);
        if (!CAIO_HASERROR(self)) {
            INFO("bar yields: %d", value);
        }
        else if (CAIO_ISERROR(self, ECANCELED)) {
            INFO("bar stopped");
            CAIO_CLEARERROR(self);
            break;
        }
        else {
            INFO("Unknowd error: %d", self->eno);
            break;
        }
    }
    INFO("foo called %d times.", foo.count);
    INFO("bar called %d times.", bar.count);
    CORO_FINALLY;
}


int
main() {
    return CAIO_FOREVER(consumerA, NULL, 1);
}
