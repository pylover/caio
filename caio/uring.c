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
#include <sys/uring.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "caio/uring.h"


struct caio_uring {
    struct caio_module;
    int fd;
    int timeout_ms;
    size_t maxevents;
    size_t waitingfiles;
};


static int
_tick(struct caio_uring *e, caio_t c) {
    return -1;
}


struct caio_uring *
caio_uring_create(caio_t c, size_t maxevents, unsigned int timeout_ms) {
    struct caio_uring *e;

    if (maxevents == 0) {
        return NULL;
    }

    /* Create uring instance */
    e = malloc(sizeof(struct caio_uring));
    if (e == NULL) {
        return NULL;
    }
    memset(e, 0, sizeof(struct caio_uring));

    e->timeout_ms = timeout_ms;
    e->waitingfiles = 0;
    e->maxevents = maxevents;
    e->fd = uring_create1(0);
    if (e->fd < 0) {
        goto failed;
    }

    e->events = calloc(e->maxevents, sizeof(struct uring_event));
    if (e->events == NULL) {
        goto failed;
    }

    e->tick = (caio_hook) _tick;

    if (caio_module_install(c, (struct caio_module*)e)) {
        goto failed;
    }

    return e;

failed:
    if (e->events) {
        free(e->events);
    }

    free(e);
    return NULL;
}


int
caio_uring_destroy(caio_t c, struct caio_uring *e) {
    int ret = 0;

    if (e == NULL) {
        return -1;
    }

    ret |= caio_module_uninstall(c, (struct caio_module*)e);

    if (e->fd != -1) {
        close(e->fd);
    }

    if (e->events) {
        free(e->events);
    }

    free(e);
    return 0;
}
