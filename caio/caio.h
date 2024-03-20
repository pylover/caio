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
#ifndef CAIO_CAIO_H_
#define CAIO_CAIO_H_


#include <errno.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>


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


enum caio_flags {
    CAIO_NONE = 0,
    CAIO_SIG = 1,
};


enum caio_taskstatus {
    CAIO_IDLE = 1,
    CAIO_RUNNING = 2,
    CAIO_TERMINATING = 4,
    CAIO_TERMINATED = 8,
    // TODO: Define inside epoll or io_uring sections
    CAIO_EPOLL_WAITING = 16,
    CAIO_WAITINGURING = 32,
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
    enum caio_taskstatus status;
    int eno;
    struct caio_call *current;
};


void
caio_invoker_default(struct caio_task *task);


int
caio_init(size_t maxtasks, int flags);


int
caio_deinit();


struct caio_task *
caio_task_new();


int
caio_task_dispose(struct caio_task *task);


void
caio_task_killall();


int
caio_loop();


int
caio_handover();


/* epoll stuff */
int
caio_epoll_register(struct caio_task *task, int fd, int events);


int
caio_epoll_unregister(int fd);


#define CAIO_EPOLL_WAIT(task, fd, events) \
    do { \
        (task)->current->line = __LINE__; \
        if (caio_epoll_register(task, fd, events)) { \
            (task)->status = CAIO_TERMINATING; \
        } \
        else { \
            (task)->status = CAIO_EPOLL_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#define CAIO_EPOLL_MUSTWAIT() \
    ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINPROGRESS))


/* IO Uring Stuff */
#include <liburing.h>
#ifndef CAIO_URING_MAXRINGS
#define CAIO_URING_MAXRINGS 8
#endif  // CAIO_URING_MAXRINGS


int
caio_uring_register(struct io_uring *u);


int
caio_uring_unregister(struct io_uring *u);


#define CAIO_URING_WAIT(u, task) \
    do { \
        (task)->current->line = __LINE__; \
        if (caio_file_monitor(task, fd, events)) { \
            (task)->status = CAIO_TERMINATING; \
        } \
        else { \
            (task)->status = CAIO_URING_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#endif  // CAIO_CAIO_H_
