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



#define ASYNC void


#define CORO_START \
    switch ((self)->current->line) { \
        case 0:


#define CORO_FINALLY \
    } \
    caiocoro_finally: \
    (self)->current->line = __LINE__; \
    (self)->status = CAIO_DONE; \
    (self)->value = NULL;


#define CORO_YIELD(v) \
    do { \
        (self)->current->line = __LINE__; \
        (self)->status = CAIO_DONE; \
        (self)->value = v; \
        return; \
        case __LINE__:; \
    } while (0)


#define CORO_YIELDFROM(coro, state, v) \
    do { \
        (self)->current->line = __LINE__; \
        if (caio_call_new(self, (caio_coro)coro, (void *)state)) { \
            (self)->status = CAIO_ERROR; \
        } \
        else { \
            (self)->status = CAIO_AGAIN; \
        } \
        return; \
        case __LINE__: \
        if (v != NULL) { \
            v = self->value; \
        } \
    } while (0)


#define CORO_WAIT(coro, state) \
    do { \
        (self)->current->line = __LINE__; \
        if (caio_call_new(self, (caio_coro)coro, (void *)state)) { \
            (self)->status = CAIO_ERROR; \
        } \
        else { \
            (self)->status = CAIO_AGAIN; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#define CORO_RUN(coro, state) \
    caio_task_new((caio_coro)coro, (void *)(state));


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
    struct caio_call *parent;
    void *state;
};


struct caio_task {
    int index;
    enum caio_corostatus status;
    struct caio_call *current;
    void *value;
};


struct caio_taskpool {
    struct caio_task **pool;
    size_t size;
    size_t count;
};


int
caio_init(size_t maxtasks);


int
caio_call_new(struct caio_task *task, caio_coro coro, void *state);


int
caio_task_new(caio_coro coro, void *state);


int
caio_forever();


#endif  // CAIO_H_
