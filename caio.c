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

#include <clog.h>

#include "caio.h"
#include "taskpool.h"


static struct caio_taskpool _tasks;


int
caio_init(size_t maxtasks) {
    if (taskpool_init(&_tasks, maxtasks)) {
        return -1;
    }

    return 0;
}


void
caio_deinit() {
    taskpool_deinit(&_tasks);
}


static void
_caio_task_dispose(struct caio_task *task) {
    taskpool_vacuumflag(&_tasks, task->index);
    free(task);
}


int
caio_task_new(caio_coro coro, void *state) {
    int index;
    struct caio_task *task;

    if (TASKPOOL_ISFULL(&_tasks)) {
        return -1;
    }

    task = malloc(sizeof(struct caio_task));
    if (task == NULL) {
        return -1;
    }

    /* Register task */
    index = taskpool_append(&_tasks, task);
    if (index == -1) {
        _caio_task_dispose(task);
        return -1;
    }
    task->index = index;
    task->current = NULL;

    /* Update the task->current */
    if (caio_call_new(task, coro, state)) {
        _caio_task_dispose(task);
        return -1;
    }

    return 0;
}


int
caio_call_new(struct caio_task *task, caio_coro coro, void *state) {
    struct caio_call *parent = task->current;
    struct caio_call *call = malloc(sizeof(struct caio_call));
    if (call == NULL) {
        return -1;
    }

    if (parent == NULL) {
        call->parent = NULL;
    }
    else {
        call->parent = parent;
    }

    call->coro = coro;
    call->state = state;
    call->line = 0;

    task->status = CAIO_AGAIN;
    task->current = call;
    return 0;
}


bool
caio_task_step(struct caio_task *task) {
    struct caio_call *call = task->current;

    /* Get a shot of whiskey to coro */
    call->coro(task, call->state);
    switch (task->status) {
        case CAIO_DONE:
        case CAIO_ERROR:
            task->current = call->parent;
            free(call);
            if (task->current != NULL) {
                task->status = CAIO_AGAIN;
            }
            break;
        case CAIO_AGAIN:
            break;
    }

    if (task->current == NULL) {
        // TODO: Error handling
        _caio_task_dispose(task);
        return true;
    }

    return false;
}


int
caio_forever() {
    int taskindex;
    struct caio_task *task = NULL;
    bool vacuum_needed;

    while (_tasks.count) {
        vacuum_needed = false;

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

    caio_deinit();
    return EXIT_SUCCESS;
}
