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

    clock_gettime(CLOCK_MONOTONIC, &now);
    diff_us = timediff(task->fdmon_timestamp, now);
    return task->fdmon_timeout_us - diff_us;
}


void
fdmon_tasks_timeout_check(struct caio *c) {
    struct timespec now;
    long diff_us;
    struct caio_task *task = NULL;

    clock_gettime(CLOCK_MONOTONIC, &now);
    while ((task = caio_task_next(c, task, CAIO_WAITING))) {
        diff_us = timediff(task->fdmon_timestamp, now);
        task->fdmon_timeout_us -= diff_us;
        if (task->fdmon_timeout_us < 0) {
            task->status = CAIO_RUNNING;
        }
    }
}
