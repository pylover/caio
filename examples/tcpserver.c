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
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include <mrb.h>

#include "caio.h"
#include "addr.h"


/* TCP server caio state and */
typedef struct tcpserver {
    const char *bindaddr;
    unsigned short bindport;
    int backlog;
} tcpserver;


/* TCP connection state types */
typedef struct tcpconn {
    int fd;
    struct sockaddr localaddr;
    struct sockaddr remoteaddr;
    mrb_t buff;
} tcpconn;



#define PAGESIZE 4096
#define BUFFSIZE (PAGESIZE * 32768)


ASYNC
tcpserverA(struct caio_task *self, struct tcpserver *state) {
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr bindaddr;
    struct sockaddr connaddr;
    static int fd;
    int connfd;
    int res;
    int option = 1;
    CORO_START;

    /* Parse listen address */
    sockaddr_parse(&bindaddr, state->bindaddr, state->bindport);

    /* Create socket */
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    /* Allow reuse the address */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* Bind to tcp port */
    res = bind(fd, &bindaddr, sizeof(bindaddr));
    if (res) {
        CORO_REJECT("Cannot bind on: %s", sockaddr_dump(&bindaddr));
    }

    /* Listen */
    res = listen(fd, state->backlog);
    INFO("Listening on: %s", sockaddr_dump(&bindaddr));
    if (res) {
        CORO_REJECT("Cannot listen on: %s", sockaddr_dump(&bindaddr));
    }

    while (true) {
        connfd = accept4(fd, &connaddr, &addrlen, SOCK_NONBLOCK);
        if ((connfd == -1) && CORO_MUSTWAIT()) {
            CORO_WAITFD(fd, EPOLLIN | EPOLLET);
            continue;
        }

        if (connfd == -1) {
            CORO_REJECT("accept4");
        }

        /* New Connection */
        INFO("New connection from: %s", sockaddr_dump(&connaddr));
        // struct tcpconn *c = malloc(sizeof(struct tcpconn));
        // if (c == NULL) {
        //     CORO_REJECT("Out of memory");
        // }

        // c->fd = connfd;
        // c->localaddr = bindaddr;
        // c->remoteaddr = connaddr;
        // c->buff = mrb_create(BUFFSIZE);
        // tcpconn_coro_create_and_run(echoA, c);
    }

    CORO_FINALLY;
    close(fd);
}


int
main() {
    struct tcpserver state = {
        .bindaddr = "0.0.0.0",
        .bindport = 3030,
        .backlog = 2,
    };

    if (caio_init(5, CAIO_SIG)) {
        return EXIT_FAILURE;
    }

    CORO_RUN(tcpserverA, &state);

    return caio_forever();
}


// static void
// echoA(struct tcpconn_coro *self, struct tcpconn *conn) {
//     ssize_t bytes;
//     struct mrb *buff = conn->buff;
//     CORO_START;
//     static int e = 0;
//     INFO("New conn: %s", sockaddr_dump(&conn->remoteaddr));
//
//     while (true) {
//         e = CET;
//
//         /* tcp write */
//         /* Write as mush as possible until EAGAIN */
//         while (!mrb_isempty(buff)) {
//             bytes = mrb_writeout(buff, conn->fd, mrb_used(buff));
//             if ((bytes == -1) && CMUSTWAIT()) {
//                 e |= COUT;
//                 break;
//             }
//             if (bytes == -1) {
//                 CORO_REJECT("write(%d)", conn->fd);
//             }
//             if (bytes == 0) {
//                 CORO_REJECT("write(%d) EOF", conn->fd);
//             }
//         }
//
//         /* tcp read */
//         /* Read as mush as possible until EAGAIN */
//         while (!mrb_isfull(buff)) {
//             bytes = mrb_readin(buff, conn->fd, mrb_available(buff));
//             if ((bytes == -1) && CMUSTWAIT()) {
//                 e |= CIN;
//                 break;
//             }
//             if (bytes == -1) {
//                 CORO_REJECT("read(%d)", conn->fd);
//             }
//             if (bytes == 0) {
//                 CORO_REJECT("read(%d) EOF", conn->fd);
//             }
//         }
//
//         /* reset errno and rewait events if neccessary */
//         errno = 0;
//         if (mrb_isempty(buff) || (e & COUT)) {
//             CORO_WAIT(conn->fd, e);
//         }
//     }
//
//     CORO_FINALLY;
//     if (conn->fd != -1) {
//         tcpconn_evloop_unregister(conn->fd);
//         close(conn->fd);
//     }
//     if (mrb_destroy(conn->buff)) {
//         ERROR("Cannot dispose buffers.");
//     }
//     free(conn);
// }
//
//
// static void
// listenA(struct tcpserver_coro *self, struct tcpserver *state) {
// }
