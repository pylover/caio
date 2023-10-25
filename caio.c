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

#include "caio.h"
#include "taskpool.h"


static int _epollfd = -1;
static int _evloop_pendingtasks = 0;
static bool _killing = false;
static struct caio_taskpool _tasks;
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
    if (_epollfd != -1) {
        return -1;
    }

    if ((flags & CAIO_SIG) && (caio_handleinterrupts())) {
        goto onerror;
    }

    /* Create epoll instance */
    _epollfd = epoll_create1(0);
    if (_epollfd < 0) {
        goto onerror;
    }

    /* Initialize task pool */
    if (taskpool_init(&_tasks, maxtasks)) {
        goto onerror;
    }

    return 0;

onerror:
    caio_deinit();
    return -1;
}


void
caio_deinit() {
    if (_epollfd != -1) {
        close(_epollfd);
        _epollfd = -1;
    }
    taskpool_deinit(&_tasks);
    errno = 0;
}


void
caio_task_dispose(struct caio_task *task) {
    taskpool_delete(&_tasks, task->index);
    free(task);
}


struct caio_task *
caio_task_new() {
    int index;
    struct caio_task *task;

    if (TASKPOOL_ISFULL(&_tasks)) {
        return NULL;
    }

    task = malloc(sizeof(struct caio_task));
    if (task == NULL) {
        return NULL;
    }

    /* Register task */
    index = taskpool_append(&_tasks, task);
    if (index == -1) {
        free(task);
        return NULL;
    }
    task->index = index;
    task->current = NULL;

    return task;
}


int
caio_evloop_register(struct caio_task *task, int fd, int events) {
    struct epoll_event ee;

    ee.events = events | EPOLLONESHOT;
    ee.data.ptr = task;
    if (epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ee)) {
        if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, fd, &ee)) {
            return -1;
        }
        errno = 0;
    }

    _evloop_pendingtasks++;
    return 0;
}


int
caio_evloop_unregister(int fd) {
    if (epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, NULL)) {
        return -1;
    }
    return 0;
}


static int
caio_evloop_wait(int timeout) {
    int nfds;
    int i;
    struct epoll_event events[_tasks.count];
    struct caio_task *task;

    nfds = epoll_wait(_epollfd, events, _tasks.count, timeout);
    if (nfds < 0) {
        return -1;
    }

    if (nfds == 0) {
        return 0;
    }

    for (i = 0; i < nfds; i++) {
        task = (struct caio_task*)events[i].data.ptr;
        if (task->status == CAIO_WAITINGIO) {
            task->status = CAIO_RUNNING;
        }
        _evloop_pendingtasks--;
    }

    return 0;
}


void
caio_task_killall() {
    int taskindex;
    struct caio_task *task;

    for (taskindex = 0; taskindex < _tasks.count; taskindex++) {
        task = taskpool_get(&_tasks, taskindex);
        if (task == NULL) {
            continue;
        }

        if (task->status == CAIO_WAITINGIO) {
            _evloop_pendingtasks--;
        }
        task->status = CAIO_TERMINATING;
    }
}


static void
caio_invoker_default(struct caio_task *task) {
    struct caio_call *call = task->current;

    call->coro(task, call->state);
}


int
caio_call_new(struct caio_task *task, caio_coro coro, void *state) {
    struct caio_call *call = malloc(sizeof(struct caio_call));
    if (call == NULL) {
        return -1;
    }

    call->parent = task->current;
    call->coro = coro;
    call->state = state;
    call->line = 0;
    call->invoke = caio_invoker_default;

    task->status = CAIO_RUNNING;
    task->current = call;
    return 0;
}


bool
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
        case CAIO_YIELDING:
            if (call->parent == NULL) {
                task->status = CAIO_RUNNING;
                break;
            }
        case CAIO_TERMINATED:
            task->current = call->parent;
            free(call);
            if (task->current != NULL) {
                task->status = CAIO_RUNNING;
            }
            break;
        default:
    }

    if (task->current == NULL) {
        caio_task_dispose(task);
        return true;
    }

    return false;
}


int
caio_loop() {
    int taskindex;
    int evloop_timeout;
    struct caio_task *task = NULL;
    bool vacuum_needed;

    while (_tasks.count) {
        vacuum_needed = false;

        if (_evloop_pendingtasks) {
            if (_evloop_pendingtasks == _tasks.count) {
                /* Wait forever */
                evloop_timeout = -1;
            }
            else {
                /* No Wait */
                evloop_timeout = 0;
            }

            if (caio_evloop_wait(evloop_timeout)) {
                if (_killing) {
                    errno = 0;
                }
                else {
                    return -1;
                }
            }
        }

        for (taskindex = 0; taskindex < _tasks.count; taskindex++) {
            task = taskpool_get(&_tasks, taskindex);
            if (task == NULL) {
                continue;
            }

            vacuum_needed |= caio_task_step(task);
        }

        if (vacuum_needed) {
            taskpool_vacuum(&_tasks);
        }
    }

    return 0;
}


int
caio_spawn(caio_coro coro, void *state) {
    struct caio_task *task = NULL;

    task = caio_task_new();
    if (task == NULL) {
        return -1;
    }

    if (caio_call_new(task, coro, state)) {
        goto failure;
    }

    return 0;

failure:
    caio_task_dispose(task);
    return -1;
}


int
caio_start() {
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
caio_forever(caio_coro coro, void *state, size_t maxtasks) {
    if (caio_init(maxtasks, CAIO_SIG)) {
        return -1;
    }

    if (caio_spawn(coro, state)) {
        goto failure;
    }

    if (caio_loop()) {
        goto failure;
    }

    caio_deinit();
    return 0;

failure:
    caio_deinit();
    return -1;
}
