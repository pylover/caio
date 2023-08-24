// Copyright 2023 Vahid Mardani
/*
 * This file is part of Carrow.
 *  Carrow is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  Carrow is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Carrow. If not, see <https://www.gnu.org/licenses/>.
 *
 *  Author: Vahid Mardani <vahid.mardani@gmail.com>
 */
#include <stdlib.h>
#include <unistd.h>

#include <clog.h>

#include "caio.h"


static ASYNC
fooA(struct caio_task *self) {
    CORO_START;

    INFO("Waiting a moment");
    static struct caio_sleep sleep = {.seconds = 5};
    CORO_WAIT(sleepA, &sleep);
    INFO("Wait done");
    CORO_FINALLY;
}


int
main() {
    if (caio_init(2, CAIO_SIG)) {
        return EXIT_FAILURE;
    }

    CORO_RUN(fooA, NULL);

    return caio_forever();
}
