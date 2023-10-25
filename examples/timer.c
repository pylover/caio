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
#include <unistd.h>

#include "caio.h"


struct timer {
    int fd;
    unsigned int interval;
    unsigned long value;
    const char *title;
};


static int
maketimer(unsigned int interval) {
    int fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd == -1) {
        return -1;
    }

    struct timespec sec1 = {interval, 0};
    struct itimerspec spec = {sec1, sec1};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        return -1;
    }
    return fd;
}


static ASYNC
timerA(struct caio_task *self, struct timer *state) {
    CORO_START;
    unsigned long tmp;
    ssize_t bytes;

    state->fd = maketimer(state->interval);
    if (state->fd == -1) {
        CORO_REJECT("maketimer");
    }

    while (true) {
        CORO_WAITFD(state->fd, EPOLLIN);
        bytes = read(state->fd, &tmp, sizeof(tmp));
        if (bytes == -1) {
            CORO_REJECT("read");
        }
        state->value += tmp;
        if (state->value > 4) {
            break;
        }
        INFO("%s(%ds), fd: %d, value: %lu", state->title, state->interval,
                state->fd, state->value);
    }

    CORO_FINALLY;
    if (state->fd != -1) {
        close(state->fd);
    }
}


int
main() {
    struct timer foo = {
        .fd = -1,
        .title = "Foo",
        .interval = 1,
        .value = 0,
    };

    struct timer bar = {
        .fd = -1,
        .title = "Bar",
        .interval = 3,
        .value = 0,
    };

    if (caio_init(2, CAIO_SIG)) {
        return EXIT_FAILURE;
    }

    CAIO_SPAWN(timerA, &foo);
    CAIO_SPAWN(timerA, &bar);

    return caio_handover();
}
