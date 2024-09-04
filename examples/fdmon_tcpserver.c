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
 *
 *
 * An edge-triggered epoll(7) example using caio.
 */
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "caio/config.h"
#include "caio/caio.h"


#ifdef CAIO_EPOLL
#include "caio/epoll.h"
#endif

#ifdef CAIO_SELECT
#include "caio/select.h"
#endif


#define MAXCONN 8
#define BUFFSIZE 1024


static struct caio *_caio;
static struct sigaction oldaction;


/* TCP server caio state and */
typedef struct tcpserver {
    volatile int sessions;
    struct caio_fdmon *fdmon;
} tcpserver_t;


/* TCP connection state types */
typedef struct tcpconn {
    int fd;
    struct sockaddr_in localaddr;
    struct sockaddr_in remoteaddr;
    char buff[BUFFSIZE];
    size_t bufflen;
    struct tcpserver *server;
} tcpconn_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY tcpserver
#define CAIO_ARG1 struct sockaddr_in
#define CAIO_ARG2 int
#include "caio/generic.h"
#include "caio/generic.c"


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY tcpconn
#include "caio/generic.h"  // NOLINT
#include "caio/generic.c"  // NOLINT


#define ADDRFMTS "%s:%d"
#define ADDRFMTV(a) inet_ntoa((a).sin_addr), ntohs((a).sin_port)


static void
_state_print(const struct tcpserver *s) {
    printf("caio tcpserver example, active sessions: %d\n", s->sessions);
}


static void
_sighandler(int s) {
    printf("\nsignal: %d\n", s);
    caio_task_killall(_caio);
    printf("\n");
}


static int
_handlesignals() {
    struct sigaction new_action = {{_sighandler}, {{0, 0, 0, 0}}};
    if (sigaction(SIGINT, &new_action, &oldaction) != 0) {
        return -1;
    }

    return 0;
}


static ASYNC
echoA(struct caio_task *self, struct tcpconn *conn) {
    ssize_t bytes;
    struct tcpserver *server = conn->server;
    CAIO_BEGIN(self);

    while (true) {
reading:
        /* tcp read */
        bytes = read(conn->fd, conn->buff, BUFFSIZE);
        if ((bytes == -1) && IO_MUSTWAIT(errno)) {
            CAIO_FILE_AWAIT(server->fdmon, self, conn->fd, CAIO_IN);
            goto reading;
        }
        else if (bytes == -1) {
            warn("read(fd: %d)", conn->fd);
            CAIO_THROW(self, errno);
        }
        else if (bytes == 0) {
            warn("read(fd: %d) EOF", conn->fd);
            CAIO_THROW(self, errno);
        }
        conn->bufflen = bytes;

writing:
        /* tcp write */
        bytes = write(conn->fd, conn->buff, conn->bufflen);
        if ((bytes == -1) && IO_MUSTWAIT(errno)) {
            CAIO_FILE_AWAIT(server->fdmon, self, conn->fd, CAIO_OUT);
            goto writing;
        }
        else if (bytes == -1) {
            warn("write(fd: %d)", conn->fd);
            CAIO_THROW(self, errno);
        }
        else if (bytes == 0) {
            warn("write(fd: %d) EOF", conn->fd);
            CAIO_THROW(self, errno);
        }
    }

    CAIO_FINALLY(self);
    if (conn->fd != -1) {
        CAIO_FILE_FORGET(server->fdmon, conn->fd);
        close(conn->fd);
        conn->server->sessions--;
        _state_print(conn->server);
    }
    free(conn);
}


static ASYNC
listenA(struct caio_task *self, struct tcpserver *state,
        struct sockaddr_in bindaddr, int backlog) {
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr_in connaddr;
    static int fd;
    int connfd;
    int res;
    int option = 1;
    CAIO_BEGIN(self);

    /* Create socket */
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    /* Allow reuse the address */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* Bind to tcp port */
    res = bind(fd, &bindaddr, sizeof(bindaddr));
    if (res) {
        warn("Cannot bind on: "ADDRFMTS"\n", ADDRFMTV(bindaddr));
        CAIO_THROW(self, errno);
    }

    /* Listen */
    res = listen(fd, backlog);
    printf("Listening on: tcp://"ADDRFMTS" backlog: %d\n", ADDRFMTV(bindaddr),
            backlog);
    if (res) {
        warn("Cannot listen on: "ADDRFMTS"\n", ADDRFMTV(bindaddr));
        CAIO_THROW(self, errno);
    }

    while (true) {
        connfd = accept4(fd, &connaddr, &addrlen, SOCK_NONBLOCK);
        if ((connfd == -1) && IO_MUSTWAIT(errno)) {
            CAIO_FILE_AWAIT(state->fdmon, self, fd, CAIO_IN);
            continue;
        }

        if (connfd == -1) {
            warn("accept4\n");
            CAIO_THROW(self, errno);
        }

        /* New Connection */
        printf("New connection from: "ADDRFMTS"\n", ADDRFMTV(connaddr));
        struct tcpconn *c = malloc(sizeof(struct tcpconn));
        if (c == NULL) {
            warn("Out of memory\n");
            CAIO_THROW(self, errno);
        }

        c->fd = connfd;
        c->localaddr = bindaddr;
        c->remoteaddr = connaddr;
        c->server = state;
        state->sessions++;
        _state_print(state);
        if (tcpconn_spawn(_caio, echoA, c)) {
            warn("Maximum connection exceeded, fd: %d\n", connfd);
            close(connfd);
            free(c);
        }
    }

    CAIO_FINALLY(self);
    if (fd != -1) {
        CAIO_FILE_FORGET(state->fdmon, fd);
        close(fd);
    }
}


int
main() {
    int exitstatus = EXIT_SUCCESS;
    struct tcpserver state = {
        .sessions = 0,
        .fdmon = NULL
    };
    struct sockaddr_in bindaddr = {
        .sin_addr = {htons(0)},
        .sin_port = htons(3030),
    };

    if (_handlesignals()) {
        return EXIT_FAILURE;
    }

    _caio = caio_create(MAXCONN + 1);
    if (_caio == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

#ifdef CAIO_EPOLL
    struct caio_epoll *epoll;
    epoll = caio_epoll_create(_caio, MAXCONN + 1, 1);
    if (epoll == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }
    state.fdmon = (struct caio_fdmon*)epoll;
    printf("Using epoll(7) for IO monitoring.\n");

#elifdef CAIO_SELECT
    struct caio_select *select;
    select = caio_select_create(_caio, MAXCONN + 1, 1);
    if (select == NULL) {
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }
    state.fdmon = (struct caio_fdmon*)select;
    printf("Using select(2) for IO monitoring.\n");

#endif

    tcpserver_spawn(_caio, listenA, &state, bindaddr, MAXCONN);

    if (caio_loop(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

terminate:
#ifdef CAIO_EPOLL

    if (caio_epoll_destroy(_caio, epoll)) {
        exitstatus = EXIT_FAILURE;
    }

#elifdef CAIO_SELECT

    if (caio_select_destroy(_caio, select)) {
        exitstatus = EXIT_FAILURE;
    }

#endif

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    return exitstatus;
}
