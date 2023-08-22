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
    CORO_START;
    INFO("Bar generator");
    while (true) {
    }
    CORO_FINALLY;
}


ASYNC
foo(struct caio_task *self) {
    CORO_START;
    INFO("Foo consumer");
    while (true) {
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
