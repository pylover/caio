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
#include <stddef.h>

typedef struct task *taskptr;
#undef GARR_ITEMISEMPTY
#define GARR_ITEMISEMPTY(c) (c == NULL)
#undef GARR_TYPE
#define GARR_TYPE taskptr
#include "generic_array.h"
#include "generic_array.c"


static struct caio_task **_tasks = NULL;
static size_t _taskscount = 0;


#include "caio.h"


int
caio_init() {
}


int
caio_task_append(struct caio_task *task, void *state) {
    // TODO: Implement
}


int
caio_task_step(struct caio_task *task) {
    // TODO: Implement
}


int
caio_forever() {
    int taskindex;
    struct caio_task *task = NULL;


    for (taskindex = 0; taskindex < _taskscount; taskindex++) {
        task = _tasks[taskindex];
        caio_task_step(task);
    }

    return 0;
}
