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


enum caiocoro_status {
    ccs_again,
    ccs_done,
};


struct caiotask;
typedef enum caiocoro_status (*caiocoro) (struct caiotask *self, void *state);


typedef struct caiocall {
    caiocoro coro;
    void *state;
} caiocall;


#undef GSTACKTYPE
#define GSTACKTYPE caiocall
#include "generic_stack.h"


typedef struct caiotask {
    int index;
    int running_coros;
    struct caiocall_stack callstack;
} caiotask;


int
caio_init(size_t maxtasks, size_t callstacksize);


int
caio_call_new(struct caiotask *task, caiocoro coro, void *state);


int
caio_task_new(caiocoro coro, void *state);


int
caio_forever();


#endif  // CAIO_H_
