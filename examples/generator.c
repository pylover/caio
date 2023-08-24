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
#include <stdlib.h>

#include <clog.h>

#include "caio.h"


static ASYNC
qux(struct caio_task *self) {
    CORO_START;
    static int value = 0;
    while (true) {
        CORO_YIELD(value++);
    }
    CORO_FINALLY;
}


static ASYNC
bar(struct caio_task *self) {
    CORO_START;
    static int value = 0;
    while (true) {
        CORO_YIELD(value++);
    }
    CORO_FINALLY;
}


static ASYNC
baz(struct caio_task *self) {
    CORO_START;
    int value;
    while (true) {
        CORO_YIELDFROM(qux, NULL, value, int);
        CORO_YIELD(value * 2);
    }
    CORO_FINALLY;
}


static ASYNC
foo(struct caio_task *self) {
    int value;
    CORO_START;
    INFO("Foo consumer");
    while (true) {
        CORO_YIELDFROM(bar, NULL, value, int);
        INFO("Bar: %d", value);

        CORO_YIELDFROM(baz, NULL, value, int);
        INFO("Baz: %d", value);
        if (value > 5) {
            break;
        }
    }
    CORO_FINALLY;
}


int
main() {
    if (caio_init(1, 0)) {
        return EXIT_FAILURE;
    }
    CORO_RUN(foo, NULL);

    return caio_forever();
}
