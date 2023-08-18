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

#include "caio.h"


#undef GARR_TYPE
#define GARR_TYPE caiotask
#include "generic_array.h"
#include "generic_array.c"


static struct caiotask_array _tasks;


#include "caio.h"


int
caio_init(size_t maxtasks) {
    // if (caiotask_array_init(&_tasks, maxtasks)) {
    //     return -1;
    // }
}


int
caio_task_append(struct caiotask *task, void *state) {
    // TODO: Implement
}


int
caio_task_step(struct caiotask *task) {
    // TODO: Implement
}


int
caio_forever() {
    // int taskindex;
    // struct caiotask *task = NULL;

    // for (taskindex = 0; taskindex < _taskscount; taskindex++) {
    //     task = _tasks[taskindex];
    //     caio_task_step(task);
    // }

    return 0;
}
