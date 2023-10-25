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
#ifndef CAIO_H_  // NOLINT(build/header_guard)
#error "caio.h and clog.h must be imported before importing the" \
    "caio.h"
#error "And also #undef and #define CAIO_ENTITY before importing the " \
    "caio.h"
#else


#include <stdbool.h>


typedef void (*CAIO_NAME(coro)) (struct caio_task *self, CAIO_NAME(t) *state
#ifdef CAIO_ARG1
        , CAIO_ARG1 arg1
    #ifdef CAIO_ARG2
            , CAIO_ARG2 arg2
    #endif  // CAIO_ARG2
#endif  // CAIO_ARG1
        );  // NOLINT


typedef struct CAIO_NAME(call) {
    struct caio_call *parent;
    int line;
    CAIO_NAME(coro) coro;
    CAIO_NAME(t) *state;
    caio_invoker invoke;

#ifdef CAIO_ARG1
    CAIO_ARG1 arg1;
    #ifdef CAIO_ARG2
        CAIO_ARG2 arg2;
    #endif  // CAIO_ARG2
#endif  // CAIO_ARG1
} CAIO_NAME(call);



void
CAIO_NAME(invoker)(struct caio_task *task);



int
CAIO_NAME(call_new)(struct caio_task *task, CAIO_NAME(coro) coro,
        CAIO_NAME(t) *state
#ifdef CAIO_ARG1
        , CAIO_ARG1 arg1
    #ifdef CAIO_ARG2
            , CAIO_ARG2 arg2
    #endif  // CAIO_ARG2
#endif  // CAIO_ARG1
        );  // NOLINT


int
CAIO_NAME(spawn) (CAIO_NAME(coro) coro, CAIO_NAME(t) *state
#ifdef CAIO_ARG1
        , CAIO_ARG1 arg1
    #ifdef CAIO_ARG2
            , CAIO_ARG2 arg2
    #endif  // CAIO_ARG2
#endif  // CAIO_ARG1
        );  // NOLINT


int
CAIO_NAME(forever) (CAIO_NAME(coro) coro, CAIO_NAME(t) *state
#ifdef CAIO_ARG1
        , CAIO_ARG1 arg1
    #ifdef CAIO_ARG2
            , CAIO_ARG2 arg2
    #endif  // CAIO_ARG2
#endif  // CAIO_ARG1
        , size_t maxtasks, int flags);

#endif  // CAIO_H_
