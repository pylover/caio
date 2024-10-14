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
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include "caio/caio.h"
#include "caio/taskpool.h"


struct caio {
    struct caio_taskpool taskpool;
    volatile bool terminating;
#ifdef CAIO_MODULES
    struct caio_module *modules[CAIO_MODULES_MAX];
    size_t modulescount;
#endif  // CAIO_MODULES
};


struct caio*
caio_create(size_t maxtasks) {
    struct caio *c = malloc(sizeof(struct caio));
    if (c == NULL) {
        return NULL;
    }

    c->terminating = false;

#ifdef CAIO_MODULES
    c->modulescount = 0;
#endif  // CAIO_MODULES

    /* Initialize task pool */
    if (caio_taskpool_init(&c->taskpool, maxtasks)) {
        goto onerror;
    }

    return c;

onerror:
    caio_destroy(c);
    return NULL;
}


int
caio_destroy(struct caio *c) {
    if (c == NULL) {
        return -1;
    }

    if (caio_taskpool_deinit(&c->taskpool)) {
        return -1;
    }

    free(c);
    errno = 0;
    return 0;
}


struct caio_task *
caio_task_new(struct caio *c) {
    struct caio_task *task;

    /* Register task */
    task = caio_taskpool_lease(&c->taskpool);
    if (task == NULL) {
        return NULL;
    }

    task->caio = c;
    return task;
}


int
caio_task_dispose(struct caio_task *task) {
    return caio_taskpool_release(&(task->caio->taskpool), task);
}


void
caio_task_killall(struct caio *c) {
    struct caio_task *task = NULL;

    while ((task = caio_taskpool_next(&c->taskpool, task,
                    CAIO_RUNNING | CAIO_WAITING))) {
        task->status = CAIO_TERMINATING;
    }
}


#ifdef CAIO_MODULES

int
caio_module_install(struct caio *c, struct caio_module *m) {
    if (c->modulescount == CAIO_MODULES_MAX) {
        return -1;
    }

    if ((c == NULL) || (m == NULL)) {
        return -1;
    }

    c->modules[c->modulescount++] = m;
    return 0;
}


int
caio_module_uninstall(struct caio *c, struct caio_module *m) {
    if (c->modulescount == 0) {
        return -1;
    }

    if ((c == NULL) || (m == NULL)) {
        return -1;
    }

    int i;
    int shift = 0;

    for (i = 0; i < c->modulescount; i++) {
        if (c->modules[i] == m) {
            shift++;
            continue;
        }

        if (!shift) {
            continue;
        }

        c->modules[i - shift] = c->modules[i];
        c->modules[i] = NULL;
    }

    c->modulescount -= shift;
    return 0;
}


#endif  // CAIO_MODULES


static inline bool
_step(struct caio_task *task) {
    struct caio_basecall *call = task->current;

start:
    /* Pre execution */
    if (task->status == CAIO_TERMINATING) {
        /* Tell coroutine to jump to the CORO_FINALLY label */
        call->line = -1;
    }

    call->invoke(task);

    /* Post execution */
    if (task->status == CAIO_TERMINATING) {
        goto start;
    }

    if (task->status == CAIO_TERMINATED) {
        task->current = call->parent;
        free(call);
        if (task->current != NULL) {
            task->status = CAIO_RUNNING;
        }
    }

    return task->current == NULL;
}


int
caio_loop(struct caio *c) {
    struct caio_task *task = NULL;
    struct caio_taskpool *taskpool = &c->taskpool;

#ifdef CAIO_MODULES
    int i;
    unsigned int modtimeout = 1000;
    struct caio_module *module;

    for (i = 0; i < c->modulescount; i++) {
        module = c->modules[i];
        if (module->loopstart && module->loopstart(c, module)) {
            return -1;
        }
    }

loop:
#endif

    while (taskpool->count) {

#ifdef CAIO_MODULES
        if (!c->terminating) {
            for (i = 0; i < c->modulescount; i++) {
                module = c->modules[i];
                if (module->tick && module->tick(c, module, modtimeout)) {
                    goto interrupt;
                }
            }
        }
#endif

        task = caio_taskpool_next(taskpool, task,
                    CAIO_RUNNING | CAIO_TERMINATING);
        if (task == NULL) {
#ifdef CAIO_MODULES
            modtimeout = CAIO_MODULES_TICKTIMEOUT_LONG_US / c->modulescount;
#endif
            continue;
        }

        do {
            if (_step(task)) {
                caio_taskpool_release(taskpool, task);
            }
        } while ((task = caio_taskpool_next(taskpool, task,
                    CAIO_RUNNING | CAIO_TERMINATING)));
#ifdef CAIO_MODULES
        modtimeout = CAIO_MODULES_TICKTIMEOUT_SHORT_US;
#endif
    }

#ifdef CAIO_MODULES
    for (i = 0; i < c->modulescount; i++) {
        module = c->modules[i];
        if (module->loopend && module->loopend(c, module)) {
            return -1;
        }
    }
#endif

    return 0;

#ifdef CAIO_MODULES
interrupt:
    c->terminating = true;
    caio_task_killall(c);
    goto loop;
#endif
}
