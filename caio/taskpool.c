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
#include <string.h>

#include "taskpool.h"


#define TASK_RESET(t, s) \
    (t)->status = s; \
    (t)->eno = 0; \
    (t)->current = NULL


int
caio_taskpool_release(struct caio_taskpool *pool, struct caio_task *task) {
    if (pool == NULL) {
        return -1;
    }

    if (task == NULL) {
        return -1;
    }

    TASK_RESET(task, CAIO_IDLE);
    pool->count--;
    return 0;
}


struct caio_task *
caio_taskpool_next(struct caio_taskpool *pool, struct caio_task *task,
        enum caio_taskstatus statuses) {
    if (task == NULL) {
        task = pool->tasks;
    }

    while (task <= pool->last) {
        if (task->status & statuses) {
            return task;
        }

        task++;
    }

    return NULL;
}


struct caio_task *
caio_taskpool_lease(struct caio_taskpool *pool) {
    struct caio_task *task = caio_taskpool_next(pool, NULL, CAIO_IDLE);
    if (task == NULL) {
        return NULL;
    }

    TASK_RESET(task, CAIO_RUNNING);
    pool->count++;

    return task;
}


int
caio_taskpool_init(struct caio_taskpool *pool, size_t size) {
    struct caio_task *task = NULL;

    if (pool == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (size < 1) {
        errno = EINVAL;
        return -1;
    }

    pool->tasks = calloc(size, sizeof(struct caio_task));
    if (pool->tasks == NULL) {
        return -1;
    }
    pool->last = pool->tasks + (size - 1);
    memset(pool->tasks, 0, size * sizeof(struct caio_task));
    task = pool->tasks;
    while (task <= pool->last) {
        TASK_RESET(task, CAIO_IDLE);
        task++;
    }

    pool->count = 0;
    pool->size = size;
    return 0;
}


int
caio_taskpool_destroy(struct caio_taskpool *pool) {
    if (pool == NULL) {
        return -1;
    }

    if (pool->tasks != NULL) {
        free(pool->tasks);
    }

    return 0;
}
