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
#ifndef CAIO_H_
#define CAIO_H_


#include <errno.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include <clog.h>


/* Generic stuff */
#define CAIO_NAME_PASTER(x, y) x ## _ ## y
#define CAIO_NAME_EVALUATOR(x, y)  CAIO_NAME_PASTER(x, y)
#define CAIO_NAME(n) CAIO_NAME_EVALUATOR(CAIO_ENTITY, n)

#define ASYNC void
#define CAIO_AWAIT(task, entity, coro, ...) \
    do { \
        (task)->current->line = __LINE__; \
        if (entity ## _call_new(task, coro, __VA_ARGS__)) { \
            (task)->status = CAIO_TERMINATING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#define CAIO_BEGIN(task) \
    switch ((task)->current->line) { \
        case 0:


#define CAIO_FINALLY(task) \
        case -1:; } \
    (task)->status = CAIO_TERMINATED


#define CAIO_RETURN(task) \
    (task)->eno = 0; \
    (task)->status = CAIO_TERMINATING; \
    return


#define CAIO_THROW(task, n) \
    (task)->eno = n; \
    (task)->status = CAIO_TERMINATING; \
    return


#define CAIO_RETHROW(task) \
    (task)->status = CAIO_TERMINATING; \
    return


#define CAIO_HASERROR(task) (task->eno != 0)
#define CAIO_ISERROR(task, e) (CAIO_HASERROR(task) && (task->eno == e))
#define CAIO_CLEARERROR(task) task->eno = 0


#define CAIO_WAITFD(task, fd, events) \
    do { \
        (task)->current->line = __LINE__; \
        if (caio_evloop_register(task, fd, events)) { \
            (task)->status = CAIO_TERMINATING; \
        } \
        else { \
            (task)->status = CAIO_WAITINGIO; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#define CAIO_MUSTWAITFD() \
    ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINPROGRESS))


// #define CAIO_SPAWN(coro, state) caio_spawn((caio_coro)(coro), (void*)(state))
// #define CAIO_FOREVER(coro, state, maxtasks) caio_forever((caio_coro)(coro), (void*)(state), maxtasks)


enum caio_flags {
    CAIO_NONE = 0,
    CAIO_SIG = 1,
};


enum caio_taskstatus {
    CAIO_RUNNING,
    CAIO_WAITINGIO,
    CAIO_TERMINATING,
    CAIO_TERMINATED,
};


enum caio_fdflags {
    CAIO_IN = EPOLLIN,
    CAIO_OUT = EPOLLOUT,
    CAIO_RDHUP = EPOLLRDHUP,
    CAIO_PRI = EPOLLPRI,
    CAIO_ERR = EPOLLERR,
    CAIO_HUP = EPOLLHUP,
    CAIO_ET = EPOLLET,
    CAIO_ONESHOT = EPOLLONESHOT,
    CAIO_WAKEUP = EPOLLWAKEUP,
    CAIO_EXCLUSIVE = EPOLLEXCLUSIVE,
};


typedef struct caio_task caiotask_t;
typedef void (*caio_coro) (struct caio_task *self, void *state);
typedef void (*caio_invoker) (struct caio_task *self);


struct caio_basecall {
    struct caio_call *parent;
    int line;
    caio_invoker invoke;
};


struct caio_call {
    struct caio_basecall;
    caio_coro coro;
    void *state;
};


struct caio_task {
    int index;
    int eno;
    enum caio_taskstatus status;
    struct caio_call *current;
};


int
caio_handover();


int
caio_init(size_t maxtasks, int flags);


void
caio_deinit();


int
caio_loop();


struct caio_task *
caio_task_new();


void
caio_task_dispose(struct caio_task *task);


void
caio_task_killall();


int
caio_evloop_register(struct caio_task *task, int fd, int events);


int
caio_evloop_unregister(int fd);


void
caio_invoker_default(struct caio_task *task);


#endif  // CAIO_H_
