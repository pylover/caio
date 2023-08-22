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


ASYNC
bar(struct caio_task *self) {
    static int value = 0;
    CORO_START;
    INFO("Bar generator");
    while (true) {
        CORO_YIELD(value++);
    }
    CORO_FINALLY;
}


ASYNC
foo(struct caio_task *self) {
    int value;
    CORO_START;
    INFO("Foo consumer");
    while (true) {
        CORO_YIELDFROM(bar, NULL, &value);
        // CORO_WAIT(bar, NULL);
        // value = self->value;
        INFO("value: %d", value);
        if (value > 5) {
            return CAIO_DONE;
        }
    }
    CORO_FINALLY;
}


int
main() {
    if (caio_init(2, 2)) {
        return EXIT_FAILURE;
    }
    CORO_RUN(foo, NULL);

    return caio_forever();
}
