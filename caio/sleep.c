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
#include <unistd.h>
#include <errno.h>

#include "caio/caio.h"
#include "caio/sleep.h"


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY caio_sleep
#define CAIO_ARG1 time_t
#include "caio/generic.c"


int
caio_sleep_create(caio_sleep_t *sleep) {
    int fd;
    if (sleep == NULL) {
        return -1;
    }
    fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd == -1) {
        return -1;
    }

    *sleep = fd;
    return 0;
}


int
caio_sleep_destroy(caio_sleep_t *sleep) {
    if (sleep == NULL) {
        return -1;
    }

    return close(*sleep);
}


#ifdef CAIO_SELECT

ASYNC
caio_sleepA(struct caio_task *self, int *state, caio_module_t s,
        time_t miliseconds) {
    int eno;
    int fd = *state;
    CAIO_BEGIN(self);

    if (fd == -1) {
        CAIO_THROW(self, EINVAL);
    }
    struct timespec sec = {miliseconds / 1000, (miliseconds % 1000) * 1000};
    struct timespec zero = {0, 0};
    struct itimerspec spec = {zero, sec};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        eno = errno;
        close(fd);
        *state = -1;
        CAIO_THROW(self, eno);
    }

    CAIO_MODULE_WAIT(s, self, fd, CAIO_READ);
    caio_select_forget(s, fd);
    CAIO_FINALLY(self);
}

#endif
