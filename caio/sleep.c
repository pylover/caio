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
#define CAIO_ARG1 caio_module_t
#define CAIO_ARG2 time_t
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


static int
_settimeout(int fd, time_t miliseconds) {
    if (fd == -1) {
        errno = EINVAL;
        return -1;
    }

    struct timespec sec = {miliseconds / 1000, (miliseconds % 1000) * 1000};
    struct timespec zero = {0, 0};
    struct itimerspec spec = {zero, sec};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        return -1;
    }

    return 0;
}


#ifdef CAIO_SELECT

ASYNC
caio_sleep_selectA(struct caio_task *self, caio_sleep_t *state,
        caio_select_t s, time_t miliseconds) {
    int eno;
    int fd = *state;
    CAIO_BEGIN(self);

    if (_settimeout(fd, miliseconds)) {
        eno = errno;
        close(fd);
        *state = -1;
        CAIO_THROW(self, eno);
    }

    CAIO_AWAIT_SELECT(s, self, fd, CAIO_READ);
    caio_select_forget(s, fd);
    CAIO_FINALLY(self);
}

#endif


#ifdef CAIO_EPOLL

ASYNC
caio_sleep_epollA(struct caio_task *self, caio_sleep_t *state,
        caio_epoll_t s, time_t miliseconds) {
    int eno;
    int fd = *state;
    CAIO_BEGIN(self);

    if (_settimeout(fd, miliseconds)) {
        eno = errno;
        close(fd);
        *state = -1;
        CAIO_THROW(self, eno);
    }

    CAIO_AWAIT_EPOLL(s, self, fd, EPOLLIN);
    caio_epoll_forget(s, fd);
    CAIO_FINALLY(self);
}

#endif
