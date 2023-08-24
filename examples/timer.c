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
#include <sys/timerfd.h>
#include <sys/epoll.h>

#include <clog.h>

#include "caio.h"


typedef struct timer {
    int fd;
    unsigned int interval;
    unsigned long value;
    const char *title;
} timer;


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


static void
timerA(struct caio_task *self, struct timer *state) {
    CORO_START;
    unsigned long tmp;
    ssize_t bytes;

    state->fd = maketimer(state->interval);
    if (state->fd == -1) {
        CORO_REJECT("maketimer");
    }
    while (true) {
        CORO_WAITFD(state, state->fd, EPOLLIN);
        bytes = read(state->fd, &tmp, sizeof(tmp));
        state->value += tmp;
        if (state->value > 4) {
            break;
        }
        INFO("%s, fd: %d, value: %lu", state->title, state->fd, state->value);
    }

    close(state->fd);
    CORO_FINALLY;
}


int
main() {
    struct timer state1 = {
        .fd = -1,
        .title = "Foo",
        .interval = 1,
        .value = 0,
    };
    struct timer state2 = {
        .fd = -1,
        .title = "Bar",
        .interval = 3,
        .value = 0,
    };

    if (caio_init(2)) {
        return EXIT_FAILURE;
    }
    CORO_RUN(timerA, &state1);
    // CORO_RUN(timerA, &state2);

    return caio_forever();
}
