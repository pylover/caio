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
#include "esp_sleep.h"

#include "caio/caio.h"
#include "caio/sleep.h"


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY caio_sleep
#include "caio/generic.c"


static void
_callback(struct caio_task *task) {
    if (task && (task->status == CAIO_WAITING)) {
        task->status = CAIO_RUNNING;
    }
    ESP_ERROR_CHECK(esp_timer_delete(task->sleep));
}


void
caio_esp32_sleep(struct caio_task *task, unsigned long us) {
    const esp_timer_create_args_t oneshot_timer_args = {
            .callback = (void (*)(void *)) &_callback,
            .arg = (void*) task
    };
    task->status = CAIO_WAITING;
    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &task->sleep));
    ESP_ERROR_CHECK(esp_timer_start_once(task->sleep, us));
    void 0;
}
