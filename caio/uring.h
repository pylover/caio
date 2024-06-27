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
#ifndef CAIO_URING_H_
#define CAIO_URING_H_


#include <caio/caio.h>


typedef struct caio_uring *caio_uring_t;


caio_uring_t
caio_uring_create(caio_t c, size_t maxevents, unsigned int timeout_ms);


int
caio_uring_destroy(caio_t c, caio_uring_t e);



#endif  // CAIO_URING_H_
