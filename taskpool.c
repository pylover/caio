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

    task->status = CAIO_RUNNING;
    task->eno = 0;
    task->current = NULL;

    return task;
}


int
caio_taskpool_init(struct caio_taskpool *pool, size_t size) {
    struct caio_task *task = NULL;

    if (pool == NULL) {
        return -1;
    }

    pool->tasks = calloc(size, sizeof(struct caio_task));
    if (pool->tasks == NULL) {
        return -1;
    }
    pool->last = pool->tasks + size;

    memset(pool->tasks, 0, size * sizeof(struct caio_task));
    pool->count = 0;
    pool->size = size;

    while ((task = caio_taskpool_next(pool, task, CAIO_RUNNING |
                    CAIO_WAITINGIO | CAIO_TERMINATING | CAIO_TERMINATED))) {
        task->eno = 0;
        task->current = 0;
        task->status = CAIO_IDLE;
        task++;
    }

    return 0;
}


void
caio_taskpool_destroy(struct caio_taskpool *pool) {
    if (pool == NULL) {
        return;
    }

    if (pool->tasks != NULL) {
        free(pool->tasks);
    }
}


// int
// taskpool_append(struct caio_taskpool *self, struct caio_task *item) {
//     int i;
//
//     if (item == NULL) {
//         return -1;
//     }
//
//     if (TASKPOOL_ISFULL(self)) {
//         return -1;
//     }
//
//     for (i = 0; i < self->size; i++) {
//         if (self->pool[i] == NULL) {
//             goto found;
//         }
//     }
//
//     /* Not found */
//     return -1;
//
// found:
//     self->pool[i] = item;
//     self->count++;
//     return i;
// }
//
//
// int
// taskpool_delete(struct caio_taskpool *self, unsigned int index) {
//     if (self->size <= index) {
//         return -1;
//     }
//
//     self->pool[index] = NULL;
//     return 0;
// }
//
//
// struct caio_task*
// taskpool_get(struct caio_taskpool *self, unsigned int index) {
//     if (self->size <= index) {
//         return NULL;
//     }
//
//     return self->pool[index];
// }
//
//
// void
// taskpool_vacuum(struct caio_taskpool *self) {
//     int i;
//     int shift = 0;
//
//     for (i = 0; i < self->count; i++) {
//         if (self->pool[i] == NULL) {
//             shift++;
//             continue;
//         }
//
//         if (!shift) {
//             continue;
//         }
//
//         self->pool[i - shift] = self->pool[i];
//         self->pool[i - shift]->index = i - shift;
//         self->pool[i] = NULL;
//     }
//
//     self->count -= shift;
// }
