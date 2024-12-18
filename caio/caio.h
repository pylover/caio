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


#include <stddef.h>

#include "caio/config.h"

#ifdef CONFIG_CAIO_ESP32
#include "esp_timer.h"
#endif


#ifdef CONFIG_CAIO_FDMON
#include <time.h>
#endif


enum caio_taskstatus {
    CAIO_IDLE = 1,
    CAIO_RUNNING = 2,
    CAIO_WAITING = 4,
    CAIO_TERMINATING = 8,
    CAIO_TERMINATED = 16,
};


struct caio;
struct caio_task;
typedef void (*caio_invoker) (struct caio_task *self);


struct caio_basecall {
    struct caio_basecall *parent;
    int line;
    caio_invoker invoke;
};


#ifdef CONFIG_CAIO_URING
    struct caio_uring_taskstate;
#endif


#ifdef CONFIG_CAIO_SEMAPHORE
    struct caio_semaphore;
#endif


struct caio_task {
    struct caio* caio;
    struct caio_basecall *current;
    enum caio_taskstatus status;
    int eno;

#ifdef CONFIG_CAIO_FDMON
    struct timespec fdmon_timestamp;
    long fdmon_timeout_us;
#endif

#ifdef CONFIG_CAIO_URING
    struct caio_uring_taskstate *uring;
#endif

#ifdef CONFIG_CAIO_SEMAPHORE
    struct caio_semaphore *semaphore;
#endif

#ifdef CONFIG_CAIO_ESP32
    esp_timer_handle_t sleep;
#endif
};


/* modules */
#ifdef CONFIG_CAIO_MODULES


struct caio_module;
typedef int (*caio_hook) (struct caio *c, struct caio_module *m);
typedef int (*caio_tick) (struct caio *c, struct caio_module *m,
        unsigned int timeout_us);
struct caio_module {
    caio_hook loopstart;
    caio_tick tick;
    caio_hook loopend;
};


int
caio_module_install(struct caio *c, struct caio_module *m);


int
caio_module_uninstall(struct caio *c, struct caio_module *m);


#endif  // CONFIG_CAIO_MODULES


struct caio*
caio_create(size_t maxtasks);


int
caio_destroy(struct caio* c);


struct caio_task *
caio_task_new(struct caio* c);


int
caio_task_dispose(struct caio_task *task);


struct caio_task *
caio_task_next(struct caio *c, struct caio_task *task,
        enum caio_taskstatus statuses);


void
caio_task_killall(struct caio* c);


int
caio_loop(struct caio* c);


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



#define CAIO_PASS(task, newstatus) \
    do { \
        (task)->current->line = __LINE__; \
        (task)->status = (newstatus); \
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


#endif  // CAIO_CAIO_H_
