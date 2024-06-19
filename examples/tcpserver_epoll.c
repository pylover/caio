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
#include <unistd.h>
#include <stdio.h>
#include <err.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "caio/caio.h"


#define BUFFSIZE 1024


/* TCP server caio state and */
typedef struct tcpserver {
    int connections_active;
    int connections_total;
} tcpserver_t;


/* TCP connection state types */
typedef struct tcpconn {
    int fd;
    struct sockaddr_in localaddr;
    struct sockaddr_in remoteaddr;
    char buff[BUFFSIZE];
    size_t bufflen;
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



static ASYNC
echoA(struct caio_task *self, struct tcpconn *conn) {
    ssize_t bytes;
    CAIO_BEGIN(self);
    static int events = 0;

    while (true) {
        events = CAIO_ET;

        /* tcp write */
        /* Write as mush as possible until EAGAIN */
        while (conn->bufflen) {
            bytes = write(conn->fd, conn->buff, conn->bufflen);
            if ((bytes == -1) && CAIO_EPOLL_MUSTWAIT()) {
                events |= CAIO_OUT;
                break;
            }
            if (bytes == -1) {
                warn("write(%d)\n", conn->fd);
                CAIO_THROW(self, errno);
            }
            if (bytes == 0) {
                warn("write(%d) EOF\n", conn->fd);
                CAIO_THROW(self, errno);
            }
            conn->bufflen -= bytes;
        }

        /* tcp read */
        /* Read as mush as possible until EAGAIN */
        while (conn->bufflen < BUFFSIZE) {
            bytes = read(conn->fd, conn->buff, BUFFSIZE - conn->bufflen);
            if ((bytes == -1) && CAIO_EPOLL_MUSTWAIT()) {
                events |= CAIO_IN;
                break;
            }
            if (bytes == -1) {
                warn("read(%d)\n", conn->fd);
                CAIO_THROW(self, errno);
            }
            if (bytes == 0) {
                warn("read(%d) EOF\n", conn->fd);
                CAIO_THROW(self, errno);
            }
        }

        /* reset errno and rewait events if neccessary */
        errno = 0;
        if ((conn->bufflen == 0) || (events & CAIO_OUT)) {
            CAIO_EPOLL_WAIT(self, conn->fd, events);
        }
    }

    CAIO_FINALLY(self);
    if (conn->fd != -1) {
        caio_epoll_unregister(conn->fd);
        close(conn->fd);
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
        if ((connfd == -1) && CAIO_EPOLL_MUSTWAIT()) {
            CAIO_EPOLL_WAIT(self, fd, CAIO_IN | CAIO_ET);
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
        if (tcpconn_spawn(echoA, c)) {
            warn("Maximum connection exceeded, fd: %d\n", connfd);
            close(connfd);
            free(c);
        }
    }

    CAIO_FINALLY(self);
    if (fd != -1) {
        caio_epoll_unregister(fd);
        close(fd);
    }
}


int
main() {
    int backlog = 2;
    struct sockaddr_in bindaddr = {
        .sin_addr = {htons(0)},
        .sin_port = htons(3030),
    };
    struct tcpserver state = {0, 0};

    return tcpserver_forever(listenA, &state, bindaddr, backlog, 5, CAIO_SIG);
}
