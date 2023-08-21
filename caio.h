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


#define THIS(cs) (cs).stack[(cs).count - 1]


#define ASYNC enum caio_corostatus


#define CORO_START \
    switch (THIS(self->callstack)->line) { \
        case 0:


#define CORO_FINALLY \
    } \
    caiocoro_finally: \
    return CAIO_DONE;


#define CAIO_AWAIT(coro, state) \
    do { \
        THIS(self->callstack)->line = __LINE__; \
        return CAIO_AGAIN; \
        case __LINE__:; \
    } while (0)



enum caio_corostatus {
    CAIO_AGAIN,
    CAIO_ERROR,
    CAIO_DONE,
};


struct caio_task;
typedef enum caio_corostatus (*caio_coro) (struct caio_task *self,
        void *state);


struct caio_call {
    caio_coro coro;
    int line;
    void *state;
};


struct caio_callstack {
    struct caio_call **stack;
    size_t size;
    size_t count;
};


struct caio_task {
    int index;
    int running_coros;
    struct caio_callstack callstack;
};


struct caio_taskpool {
    struct caio_task **pool;
    size_t size;
    size_t count;
};


int
caio_init(size_t maxtasks, size_t callstacksize);


int
caio_call_new(struct caio_task *task, caio_coro coro, void *state);


int
caio_task_new(caio_coro coro, void *state);


int
caio_forever();


#endif  // CAIO_H_
