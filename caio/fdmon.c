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
#include "caio/fdmon.h"



#define TSEMPTY(ts) (!((ts).tv_sec || (ts).tv_nsec))


// TODO: use macro instead
int
fdmon_task_timestamp_setnow(struct caio_task *task) {
    return clock_gettime(CLOCK_MONOTONIC, &task->fdmon_timestamp);
}


// TODO: use macro instead
void
fdmon_task_timestamp_clear(struct caio_task *task) {
    task->fdmon_timestamp.tv_sec = 0;
    task->fdmon_timestamp.tv_nsec = 0;
}


long
timediff(struct timespec start, struct timespec end) {
    long sec;
    long nsec;

    if ((end.tv_nsec - start.tv_nsec) < 0) {
        sec = end.tv_sec - start.tv_sec - 1;
        nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else {
        sec = end.tv_sec - start.tv_sec;
        nsec = end.tv_nsec - start.tv_nsec;
    }
    return (sec * 1000000) + (nsec / 1000);
}


long
fdmon_task_timeout_us(struct caio_task *task) {
    struct timespec now;
    long diff_us;

    if (TSEMPTY(task->fdmon_timestamp)) {
        return 0;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    diff_us = timediff(task->fdmon_timestamp, now);
    return task->fdmon_timeout_us - diff_us;
}


void
fdmon_tasks_timeout_check(struct caio *c) {
    struct timespec now;
    long diff_us;
    struct caio_task *task = NULL;
    long ttout;

    clock_gettime(CLOCK_MONOTONIC, &now);
    while ((task = caio_task_next(c, task, CAIO_WAITING))) {
        if (TSEMPTY(task->fdmon_timestamp)) {
            continue;
        }

        diff_us = timediff(task->fdmon_timestamp, now);
        ttout = task->fdmon_timeout_us - diff_us;
        if (ttout < 0) {
            task->fdmon_timeout_us = ttout;
            task->status = CAIO_RUNNING;
        }
    }
}
