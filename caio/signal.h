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
#ifndef CAIO_SIGNAL_H_
#define CAIO_SIGNAL_H_


#include <signal.h>

#include "caio/caio.h"


struct caio_signal;


struct caio_signal *
caio_signal_create(struct caio* c, sigset_t *signals);


int
caio_signal_destroy(struct caio* c, struct caio_signal *s);


#endif  // CAIO_SIGNAL_H_
