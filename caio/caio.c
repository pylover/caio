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
#include <errno.h>

#include "caio/caio.h"
#include "caio/taskpool.h"


struct caio {
    struct caio_taskpool taskpool;
    bool killing;
};


struct caio*
caio_create(size_t maxtasks) {

    struct caio *c = malloc(sizeof(struct caio));
    if (c == NULL) {
        return NULL;
    }

    /* Initialize task pool */
    if (caio_taskpool_init(&c->taskpool, maxtasks)) {
        goto onerror;
    }

    return c;

onerror:
    caio_destroy(c);
    return NULL;
}


int
caio_destroy(struct caio *c) {
    if (c == NULL) {
        return -1;
    }

    if (caio_taskpool_deinit(&c->taskpool)) {
        return -1;
    }

    free(c);
    errno = 0;
    return 0;
}


struct caio_task *
caio_task_new(struct caio *c) {
    struct caio_task *task;

    /* Register task */
    task = caio_taskpool_lease(&c->taskpool);
    if (task == NULL) {
        return NULL;
    }

    task->caio = c;
    return task;
}


int
caio_task_dispose(struct caio_task *task) {
    return caio_taskpool_release(&(task->caio->taskpool), task);
}


// void
// caio_task_killall() {
//     struct caio_task *task = NULL;
//
//     while ((task = caio_taskpool_next(&_taskpool, task,
//                     CAIO_RUNNING | CAIO_WAITING))) {
//         task->status = CAIO_TERMINATING;
//         task++;
//     }
// }


static bool
_step(struct caio_task *task) {
    struct caio_call *call = task->current;

start:
    /* Pre execution */
    switch (task->status) {
        case CAIO_TERMINATING:
            /* Tell coroutine to jump to the CORO_FINALLY label */
            call->line = -1;
            break;
        default:
    }

    call->invoke(task);

    /* Post execution */
    switch (task->status) {
        case CAIO_TERMINATING:
            goto start;
        case CAIO_TERMINATED:
            task->current = call->parent;
            free(call);
            if (task->current != NULL) {
                task->status = CAIO_RUNNING;
            }
            break;
        default:
    }

    return task->current == NULL;
}


int
caio_loop(struct caio *c) {
    struct caio_task *task = NULL;
    struct caio_taskpool *taskpool = &c->taskpool;

    // TODO: modules hook

    while (taskpool->count) {
        // TODO: modules hook

        while ((task = caio_taskpool_next(taskpool, task,
                    CAIO_RUNNING | CAIO_TERMINATING))) {
            if (_step(task)) {
                caio_taskpool_release(taskpool, task);
            }
        }

        // TODO: modules hook
    }

    return 0;
}
// //
// //
// // int
// // caio_epoll_register(struct caio_task *task, int fd, int events) {
// //     return caio_io_epoll_monitor(&_epoll, task, fd, events);
// // }
// //
// //
// // int
// // caio_epoll_unregister(int fd) {
// //     return caio_io_epoll_forget(&_epoll, fd);
// // }
