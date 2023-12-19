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

#include "caio.h"
#include "sleep.h"


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY sleep
#define CAIO_ARG1 time_t
#include "generic.c"


ASYNC
caio_sleepA(struct caio_task *self, int *state, time_t seconds) {
    int fd = *state;
    CORO_START(self);

    fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd == -1) {
        ERROR("timerfd_create");
        CORO_RETURN(self);
    }
    *state = fd;

    struct timespec sec = {seconds, 0};
    struct timespec zero = {0, 0};
    struct itimerspec spec = {zero, sec};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        close(fd);
        ERROR("timerfd_settime");
        CORO_RETURN(self);
    }

    CORO_WAITFD(self, fd, EPOLLIN);
    CORO_FINALLY(self);
    caio_evloop_unregister(fd);
    close(fd);
}
