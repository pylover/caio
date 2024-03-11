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
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include <clog.h>

#include "caio/caio.h"
#include "caio/epoll.h"
#include "caio/taskpool.h"


static bool _killing = false;
static struct caio_taskpool _taskpool;
static struct caio_epoll _epoll;
static struct sigaction old_action;

static void
_sighandler(int s) {
    _killing = true;
    caio_task_killall();
    printf("\n");
}


static int
caio_handleinterrupts() {
    struct sigaction new_action = {{_sighandler}, {{0, 0, 0, 0}}};
    if (sigaction(SIGINT, &new_action, &old_action) != 0) {
        return -1;
    }

    return 0;
}


int
caio_init(size_t maxtasks, int flags) {
    if ((flags & CAIO_SIG) && (caio_handleinterrupts())) {
        goto onerror;
    }

    /* Initialize IO monitoring backend */
    if (caio_epoll_init(&_epoll, maxtasks)) {
        goto onerror;
    }

    /* Initialize task pool */
    if (caio_taskpool_init(&_taskpool, maxtasks)) {
        goto onerror;
    }

    return 0;

onerror:
    caio_deinit();
    return -1;
}


void
caio_deinit() {
    caio_epoll_deinit(&_epoll);
    caio_taskpool_destroy(&_taskpool);
    errno = 0;
}


struct caio_task *
caio_task_new() {
    struct caio_task *task;

    /* Register task */
    task = caio_taskpool_lease(&_taskpool);
    if (task == NULL) {
        return NULL;
    }
    return task;
}


int
caio_task_dispose(struct caio_task *task) {
    return caio_taskpool_release(&_taskpool, task);
}


void
caio_task_killall() {
    struct caio_task *task = NULL;

    while ((task = caio_taskpool_next(&_taskpool, task,
                    CAIO_RUNNING | CAIO_WAITINGIO))) {
        task->status = CAIO_TERMINATING;
        task++;
    }
}


static bool
caio_task_step(struct caio_task *task) {
    struct caio_call *call = task->current;

start:

    /* Pre execution */
    switch (task->status) {
        case CAIO_TERMINATING:
            /* Tell coroutine to jump to the CORO_FINALLY label */
            call->line = -1;
            break;
        case CAIO_WAITINGIO:
            /* Ignore if task is waiting for IO events */
            return false;
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
caio_loop() {
    int epoll_timeout;
    struct caio_task *task = NULL;
    int iopending = 0;

    while (_taskpool.count) {
        if (iopending) {
            /* Check whenever all tasks are pending IO operation. */
            if (iopending == _taskpool.count) {
                /* Wait forever */
                epoll_timeout = -1;
            }
            else {
                /* No Wait */
                epoll_timeout = 0;
            }

            if (caio_epoll_wait(&_epoll, epoll_timeout)) {
                if (_killing) {
                    errno = 0;
                }
                else {
                    return -1;
                }
            }
        }

        iopending = 0;
        while ((task = caio_taskpool_next(&_taskpool, task,
                    CAIO_RUNNING | CAIO_WAITINGIO | CAIO_TERMINATING))) {
            if (task->status == CAIO_WAITINGIO) {
                iopending++;
            }
            else if (caio_task_step(task)) {
                caio_taskpool_release(&_taskpool, task);
            }
            task++;
        }
    }

    return 0;
}


int
caio_handover() {
    if (caio_loop()) {
        goto onerror;
    }

    caio_deinit();
    return 0;

onerror:
    caio_deinit();
    return -1;
}


int
caio_file_monitor(struct caio_task *task, int fd, int events) {
    return caio_epoll_monitor(&_epoll, task, fd, events);
}


int
caio_file_forget(int fd) {
    return caio_epoll_forget(&_epoll, fd);
}