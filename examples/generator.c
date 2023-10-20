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


struct state {
    const char *name;
    int count;
};
static struct state bar = {.name="bar", 0};
static struct state baz = {.name="baz", 0};
static struct state qux = {.name="qux", 0};


static ASYNC
quxA(struct caio_task *self, struct state *state) {
    CORO_START;
    state->count++;
    static int value = 0;
    while (true) {
        CORO_YIELD(value++);
    }
    CORO_FINALLY;
}


static ASYNC
barA(struct caio_task *self, struct state *state) {
    CORO_START;
    state->count++;
    static int value = 0;
    while (true) {
        CORO_YIELD(value++);
    }
    CORO_FINALLY;
}


static ASYNC
bazA(struct caio_task *self, struct state *state) {
    CORO_START;
    state->count++;
    int value;
    while (true) {
        CORO_YIELDFROM(quxA, &qux, value, int);
        CORO_YIELD(value * 2);
    }
    CORO_FINALLY;
}


static ASYNC
fooA(struct caio_task *self) {
    int value;
    CORO_START;
    INFO("Foo consumer");
    while (true) {
        CORO_YIELDFROM(barA, &bar, value, int);
        INFO("Bar yields: %d", value);

        CORO_YIELDFROM(bazA, &baz, value, int);
        INFO("Baz yields: %d", value);
        if (value > 5) {
            break;
        }
    }
    INFO("Bar called %d times.", bar.count);
    INFO("Baz called %d times.", baz.count);
    INFO("Qux called %d times.", qux.count);
    CORO_FINALLY;
}


int
main() {
    return CAIO(fooA, NULL, 1);
}
