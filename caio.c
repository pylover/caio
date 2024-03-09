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
static struct caio_taskpool _taskpool;
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
    if (_epollfd != -1) {
        close(_epollfd);
        _epollfd = -1;
    }
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
    _evloop_pendingtasks--;
    return 0;
}


static int
caio_evloop_wait(int count, int timeout) {
    int nfds;
    int i;
    struct epoll_event events[count];
    struct caio_task *task;

    nfds = epoll_wait(_epollfd, events, count, timeout);
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
    struct caio_task *task = NULL;

    while ((task = caio_taskpool_next(&_taskpool, task,
                    CAIO_RUNNING | CAIO_WAITINGIO))) {
        task->status = CAIO_TERMINATING;
        task++;
    }
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
    int evloop_timeout;
    struct caio_task *task = NULL;

    while (_taskpool.count) {
        if (_evloop_pendingtasks) {

            /* Check whenever all tasks are pending IO operation. */
            if (_evloop_pendingtasks == _taskpool.count) {
                /* Wait forever */
                evloop_timeout = -1;
            }
            else {
                /* No Wait */
                evloop_timeout = 0;
            }

            if (caio_evloop_wait(_evloop_pendingtasks, evloop_timeout)) {
                if (_killing) {
                    errno = 0;
                }
                else {
                    return -1;
                }
            }
        }

        while ((task = caio_taskpool_next(&_taskpool, task,
                    CAIO_RUNNING | CAIO_TERMINATING))) {
            if (caio_task_step(task)) {
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
