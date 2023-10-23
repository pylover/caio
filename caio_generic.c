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
 *
 *
 * An edge-triggered epoll(7) example using caio.
 */
#include "caio.h"  // NOLINT


void
CAIO_NAME(invoker)(struct caio_task *task) {
    struct CAIO_NAME(call) *call = (struct CAIO_NAME(call)*) task->current;

    call->coro(task, call->state
#ifdef CAIO_ARG1
        , call->arg1
#endif
#ifdef CAIO_ARG2
        , call->arg2
#endif
    );  // NOLINT
}



int
CAIO_NAME(call_new)(struct caio_task *task, CAIO_NAME(coro) coro,
        CAIO_NAME(t) *state
#ifdef CAIO_ARG1
        , CAIO_ARG1 arg1
#endif
#ifdef CAIO_ARG2
        , CAIO_ARG2 arg2
#endif
        ) {
    struct CAIO_NAME(call) *call;

    call = malloc(sizeof(struct CAIO_NAME(call)));
    if (call == NULL) {
        return -1;
    }

    call->parent = task->current;
    call->coro = coro;
    call->state = state;
    call->line = 0;
    call->invoke = CAIO_NAME(invoker);

    task->status = CAIO_RUNNING;
    task->current = (struct caio_call*) call;

    /* arguments */
#ifdef CAIO_ARG1
    call->arg1 = arg1;
#endif
#ifdef CAIO_ARG2
    call->arg2 = arg2;
#endif
    return 0;
}


int
CAIO_NAME(spawn) (CAIO_NAME(coro) coro, CAIO_NAME(t) *state
#ifdef CAIO_ARG1
        , CAIO_ARG1 arg1
#endif
#ifdef CAIO_ARG2
        , CAIO_ARG2 arg2
#endif
        ) {
    struct caio_task *task = NULL;

    task = caio_task_new();
    if (task == NULL) {
        return -1;
    }

    if (CAIO_NAME(call_new)(task, coro, state
#ifdef CAIO_ARG1
        , arg1
#endif
#ifdef CAIO_ARG2
        , arg2
#endif
        )) {  // NOLINT
        goto failure;
    }

    if (caio_start()) {
        goto failure;
    }

    return 0;

failure:
    caio_task_dispose(task);
    return -1;
}


int
CAIO_NAME(forever) (CAIO_NAME(coro) coro, CAIO_NAME(t) *state
#ifdef CAIO_ARG1
        , CAIO_ARG1 arg1
#endif
#ifdef CAIO_ARG2
        , CAIO_ARG2 arg2
#endif
        , size_t maxtasks, int flags) {
    if (caio_init(maxtasks, flags)) {
        return -1;
    }

    if (CAIO_NAME(spawn)(coro, state
#ifdef CAIO_ARG1
        , arg1
#endif
#ifdef CAIO_ARG2
        , arg2
#endif
        )) {  // NOLINT
        return -1;
    }

    if (caio_start()) {
        goto failure;
    }

    caio_deinit();
    return 0;

failure:
    caio_deinit();
    return -1;
}
