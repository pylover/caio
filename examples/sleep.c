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


static ASYNC
fooA(struct caio_task *self) {
    CORO_START;
    static struct caio_sleep sleep = {.seconds = 2};

    INFO("Waiting 2 seconds");
    CORO_WAIT(sleepA, &sleep);
    CORO_FINALLY;
}


int
main() {
    return CAIO_FOREVER(fooA, NULL, 2);
}
