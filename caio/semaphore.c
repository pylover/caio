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
#include "caio/semaphore.h"


int
caio_semaphore_begin(struct caio_task *task, struct caio_semaphore *s) {
    if (task->semaphore != NULL) {
        return -1;
    }

    task->semaphore = s;
    s->value = 0;
    s->task = task;
    return 0;
}


int
caio_semaphore_end(struct caio_task *task) {
    if (task->semaphore != NULL) {
        return -1;
    }

    task->semaphore = NULL;
    return 0;
}


int
caio_semaphore_acquire(struct caio_task *task) {
    if (task->semaphore == NULL) {
        return -1;
    }
    task->semaphore->value++;
    return 0;
}


int
caio_semaphore_release(struct caio_task *task) {
    if (task->semaphore == NULL) {
        return -1;
    }

    struct caio_semaphore *s = task->semaphore;
    s->value--;

    if (s->value == 0) {
        if (s->task->status == CAIO_WAITING) {
            s->task->status = CAIO_RUNNING;
        }
    }

    task->semaphore = NULL;
    return 0;
}
