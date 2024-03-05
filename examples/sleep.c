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
#include "sleep.h"

typedef void foo_t;
#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY foo
#include "generic.h"
#include "generic.c"


static ASYNC
fooA(struct caio_task *self, foo_t *) {
    CAIO_BEGIN(self);
    static int sleep;
    INFO("Waiting 2 seconds");
    CAIO_SLEEP(self, &sleep, 2);

    INFO("Waiting 3 seconds");
    CAIO_SLEEP(self, &sleep, 3);
    CAIO_FINALLY(self);
}


int
main() {
    return foo_forever(fooA, NULL, 2, CAIO_SIG);
}
