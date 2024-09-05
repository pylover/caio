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

#include <clog.h>

#include "caio/config.h"
#include "caio/caio.h"
#include "caio/uring.h"


#define MAXCONN 8
#define BUFFSIZE 1024


static struct caio *_caio;
static struct sigaction oldaction;


/* TCP server caio state and */
typedef struct tcpserver {
    volatile int sessions;
    struct caio_uring *uring;
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


// static void
// _state_print(const struct tcpserver *s) {
//     printf("caio tcpserver example, active sessions: %d\n", s->sessions);
// }


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


// static ASYNC
// echoA(struct caio_task *self, struct tcpconn *conn) {
//     ssize_t bytes;
//     struct tcpserver *server = conn->server;
//     CAIO_BEGIN(self);
//
//     while (true) {
// reading:
//         /* tcp read */
//         bytes = read(conn->fd, conn->buff, BUFFSIZE);
//         if ((bytes == -1) && IO_MUSTWAIT(errno)) {
//             CAIO_FILE_AWAIT(server->fdmon, self, conn->fd, CAIO_IN);
//             goto reading;
//         }
//         else if (bytes == -1) {
//             warn("read(fd: %d)", conn->fd);
//             CAIO_THROW(self, errno);
//         }
//         else if (bytes == 0) {
//             warn("read(fd: %d) EOF", conn->fd);
//             CAIO_THROW(self, errno);
//         }
//         conn->bufflen = bytes;
//
// writing:
//         /* tcp write */
//         bytes = write(conn->fd, conn->buff, conn->bufflen);
//         if ((bytes == -1) && IO_MUSTWAIT(errno)) {
//             CAIO_FILE_AWAIT(server->fdmon, self, conn->fd, CAIO_OUT);
//             goto writing;
//         }
//         else if (bytes == -1) {
//             warn("write(fd: %d)", conn->fd);
//             CAIO_THROW(self, errno);
//         }
//         else if (bytes == 0) {
//             warn("write(fd: %d) EOF", conn->fd);
//             CAIO_THROW(self, errno);
//         }
//     }
//
//     CAIO_FINALLY(self);
//     if (conn->fd != -1) {
//         CAIO_FILE_FORGET(server->fdmon, conn->fd);
//         close(conn->fd);
//         conn->server->sessions--;
//         _state_print(conn->server);
//     }
//     free(conn);
// }


static ASYNC
listenA(struct caio_task *self, struct tcpserver *state,
        struct sockaddr_in bindaddr, int backlog) {
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr_in connaddr;
    int connfd;
    int ret;
    int sockopt = 1;
    static int listenfd;
    CAIO_BEGIN(self);

    /* create, setup and submit a socket creation uring task */
    ret = caio_uring_socket(
            state->uring,
            self,
            AF_INET,
            SOCK_STREAM | SOCK_NONBLOCK,
            0,
            0);
    if (ret < 0) {
        perror("io_uring socket submit.");
        CAIO_THROW(self, -ret);
    }

    /* wait for socket to made */
    CAIO_URING_AWAIT(state->uring, self, 1);
    listenfd = caio_uring_cqe_get(self, 0)->res;
    if (listenfd < 0) {
        perror("io_uring socket submit.");
        CAIO_THROW(self, -listenfd);
    }
    caio_uring_cqe_seen(state->uring, self, 0);

    /* allow reuse the address */
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));

    DEBUG("listenfd: %d", listenfd);
    /* bind to tcp port */
    ret = bind(listenfd, &bindaddr, sizeof(bindaddr));
    if (ret < 0) {
        warn("Cannot bind on: "ADDRFMTS"\n", ADDRFMTV(bindaddr));
        CAIO_THROW(self, errno);
    }

    /* listen */
    ret = listen(listenfd, backlog);
    INFO("Listening on: tcp://"ADDRFMTS" backlog: %d", ADDRFMTV(bindaddr),
            backlog);
    if (ret < 0) {
        warn("Cannot listen on: "ADDRFMTS"\n", ADDRFMTV(bindaddr));
        CAIO_THROW(self, errno);
    }

    while (true) {
        ret = caio_uring_accept(
                state->uring,
                self,
                listenfd,
                (struct sockaddr *)&connaddr,
                &addrlen,
                SOCK_NONBLOCK);
        if (ret < 0) {
            perror("io_uring accept multishot submit.");
            CAIO_THROW(self, -ret);
        }

        /* wait for socket to made */
        CAIO_URING_AWAIT(state->uring, self, 1);
        connfd = caio_uring_cqe_get(self, 0)->res;
        caio_uring_cqe_seen(state->uring, self, 0);
        DEBUG("connection fd: %d", connfd);
    //     connfd = accept4(listenfd, &connaddr, &addrlen, SOCK_NONBLOCK);
    //     if ((connfd == -1) && IO_MUSTWAIT(errno)) {
    //         CAIO_FILE_AWAIT(state->fdmon, self, fd, CAIO_IN);
    //         continue;
    //     }

    //     if (connfd == -1) {
    //         warn("accept4\n");
    //         CAIO_THROW(self, errno);
    //     }

    //     /* New Connection */
    //     printf("New connection from: "ADDRFMTS"\n", ADDRFMTV(connaddr));
    //     struct tcpconn *c = malloc(sizeof(struct tcpconn));
    //     if (c == NULL) {
    //         warn("Out of memory\n");
    //         CAIO_THROW(self, errno);
    //     }

    //     c->fd = connfd;
    //     c->localaddr = bindaddr;
    //     c->remoteaddr = connaddr;
    //     c->server = state;
    //     state->sessions++;
    //     _state_print(state);
    //     if (tcpconn_spawn(_caio, echoA, c)) {
    //         warn("Maximum connection exceeded, fd: %d\n", connfd);
    //         close(connfd);
    //         free(c);
    //     }
    }

    CAIO_FINALLY(self);
    if (listenfd != -1) {
        close(listenfd);
    }
}


int
main() {
    int exitstatus = EXIT_SUCCESS;
    struct tcpserver state = {
        .sessions = 0,
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

    /* Initialize io_uring */
    // TODO: tune max uring tasks
    state.uring = caio_uring_create(_caio, MAXCONN + 1, 0, NULL);
    if (state.uring == NULL) {
        perror("io_uring setup failed!");
        exitstatus = EXIT_FAILURE;
        goto terminate;
    }

    tcpserver_spawn(_caio, listenA, &state, bindaddr, MAXCONN);

    if (caio_loop(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

terminate:
    caio_uring_destroy(_caio, state.uring);

    if (caio_destroy(_caio)) {
        exitstatus = EXIT_FAILURE;
    }

    return exitstatus;
}
