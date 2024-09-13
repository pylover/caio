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
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "caio/caio.h"
#include "caio/signal.h"


static volatile int _terminate = 0;


struct caio_signal {
    struct caio_module;
    sigset_t *signals;
    struct sigaction backups[NSIG];
};


static void
_sighandler(int s) {
    _terminate = s;
}


static int
_loopstart(struct caio *c, struct caio_signal *s) {
    int sig;
    struct sigaction newaction = {{_sighandler}, {{0, 0, 0, 0}}};

    for (sig = 1; sig < NSIG; sig++) {
        if (!sigismember(s->signals, sig)) {
            continue;
        }

        if (sigaction(sig, &newaction, &s->backups[sig])) {
            goto failed;
        }
    }

    return 0;

failed:
    /* rolling back */
    for (--sig; sig > 0; sig--) {
        if (!sigismember(s->signals, sig)) {
            continue;
        }
        sigaction(sig, &s->backups[sig], NULL);
    }

    return -1;
}


static int
_loopend(struct caio *c, struct caio_signal *s) {
    int sig;
    int ret = 0;

    /* rolling back */
    for (sig = NSIG; sig; sig--) {
        if (!sigismember(s->signals, sig)) {
            continue;
        }
        ret |= sigaction(sig, &s->backups[sig], NULL);
    }

    return ret;
}


static int
_tick(struct caio *c, struct caio_signal *s, unsigned int timeout_us) {
    return _terminate;
}


struct caio_signal *
caio_signal_create(struct caio* c, sigset_t *signals) {
    struct caio_signal *s;

    /* Create signal instance */
    s = malloc(sizeof(struct caio_signal));
    if (s == NULL) {
        return NULL;
    }

    memset(s, 0, sizeof(struct caio_signal));
    s->signals = signals;
    s->loopstart = (caio_hook) _loopstart;
    s->loopend = (caio_hook) _loopend;
    s->tick = (caio_tick) _tick;

    if (caio_module_install(c, (struct caio_module*)s)) {
        free(s);
        return NULL;
    }

    return s;
}


int
caio_signal_destroy(struct caio* c, struct caio_signal *s) {
    int ret = 0;

    if (s == NULL) {
        return -1;
    }

    ret |= caio_module_uninstall(c, (struct caio_module*)s);
    free(s);

    return ret;
}


/*
Signal      Standard   Action   Comment
SIGABRT      P1990      Core    Abort signal from abort(3)
SIGALRM      P1990      Term    Timer signal from alarm(2)
SIGBUS       P2001      Core    Bus error (bad memory access)
SIGCHLD      P1990      Ign     Child stopped or terminated
SIGCLD         -        Ign     A synonym for SIGCHLD
SIGCONT      P1990      Cont    Continue if stopped
SIGEMT         -        Term    Emulator trap
SIGFPE       P1990      Core    Floating-point exception
SIGHUP       P1990      Term    Hangup detected on controlling terminal
                                or death of controlling process
SIGILL       P1990      Core    Illegal Instruction
SIGINFO        -                A synonym for SIGPWR
SIGINT       P1990      Term    Interrupt from keyboard
SIGIO          -        Term    I/O now possible (4.2BSD)
SIGIOT         -        Core    IOT trap. A synonym for SIGABRT
SIGKILL      P1990      Term    Kill signal
SIGLOST        -        Term    File lock lost (unused)
SIGPIPE      P1990      Term    Broken pipe: write to pipe with no
                                readers; see pipe(7)
SIGPOLL      P2001      Term    Pollable event (Sys V);
                                synonym for SIGIO
SIGPROF      P2001      Term    Profiling timer expired
SIGPWR         -        Term    Power failure (System V)
SIGQUIT      P1990      Core    Quit from keyboard
SIGSEGV      P1990      Core    Invalid memory reference
SIGSTKFLT      -        Term    Stack fault on coprocessor (unused)
SIGSTOP      P1990      Stop    Stop process
SIGTSTP      P1990      Stop    Stop typed at terminal
SIGSYS       P2001      Core    Bad system call (SVr4);
                                see also seccomp(2)
SIGTERM      P1990      Term    Termination signal
SIGTRAP      P2001      Core    Trace/breakpoint trap
SIGTTIN      P1990      Stop    Terminal input for background process
SIGTTOU      P1990      Stop    Terminal output for background process
SIGUNUSED      -        Core    Synonymous with SIGSYS
SIGURG       P2001      Ign     Urgent condition on socket (4.2BSD)
SIGUSR1      P1990      Term    User-defined signal 1
SIGUSR2      P1990      Term    User-defined signal 2
SIGVTALRM    P2001      Term    Virtual alarm clock (4.2BSD)
SIGXCPU      P2001      Core    CPU time limit exceeded (4.2BSD);
                                see setrlimit(2)
SIGXFSZ      P2001      Core    File size limit exceeded (4.2BSD);
                                see setrlimit(2)
SIGWINCH       -        Ign     Window resize signal (4.3BSD, Sun)
*/
