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
#ifndef CAIO_SELECT_H_
#define CAIO_SELECT_H_


#include <sys/select.h>

#include "caio/fdmon.h"
#include "caio/caio.h"


struct caio_select;


struct caio_select *
caio_select_create(struct caio* c, size_t maxfileno);


int
caio_select_destroy(struct caio* c, struct caio_select *s);


int
caio_select_monitor(struct caio_select *s, struct caio_task *task, int fd,
        int events);


int
caio_select_forget(struct caio_select *s, int fd);


#endif  // CAIO_SELECT_H_
