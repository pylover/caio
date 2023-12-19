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
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <mrb.h>

#include "caio.h"


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
    mrb_t buff;
} tcpconn_t;


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY tcpserver
#define CAIO_ARG1 struct sockaddr_in
#define CAIO_ARG2 int
#include "generic.h"
#include "generic.c"


#undef CAIO_ARG1
#undef CAIO_ARG2
#undef CAIO_ENTITY
#define CAIO_ENTITY tcpconn
#include "generic.h"  // NOLINT
#include "generic.c"  // NOLINT


#define PAGESIZE 4096
#define BUFFSIZE (PAGESIZE * 32768)


#define ADDRFMTS "%s:%d"
#define ADDRFMTV(a) inet_ntoa((a).sin_addr), ntohs((a).sin_port)



static ASYNC
echoA(struct caio_task *self, struct tcpconn *conn) {
    ssize_t bytes;
    struct mrb *buff = conn->buff;
    CAIO_BEGIN(self);
    static int events = 0;

    while (true) {
        events = CAIO_ET;

        /* tcp write */
        /* Write as mush as possible until EAGAIN */
        while (!mrb_isempty(buff)) {
            bytes = mrb_writeout(buff, conn->fd, mrb_used(buff));
            DEBUG("writing: %d bytes: %d", conn->fd, bytes);
            if ((bytes == -1) && CAIO_MUSTWAITFD()) {
                events |= CAIO_OUT;
                break;
            }
            if (bytes == -1) {
                ERROR("write(%d)", conn->fd);
                CAIO_RETURN(self);
            }
            if (bytes == 0) {
                ERROR("write(%d) EOF", conn->fd);
                CAIO_RETURN(self);
            }
        }

        /* tcp read */
        /* Read as mush as possible until EAGAIN */
        while (!mrb_isfull(buff)) {
            bytes = mrb_readin(buff, conn->fd, mrb_available(buff));
            DEBUG("reading: %d bytes: %d", conn->fd, bytes);
            if ((bytes == -1) && CAIO_MUSTWAITFD()) {
                events |= CAIO_IN;
                break;
            }
            if (bytes == -1) {
                ERROR("read(%d)", conn->fd);
                CAIO_RETURN(self);
            }
            if (bytes == 0) {
                ERROR("read(%d) EOF", conn->fd);
                CAIO_RETURN(self);
            }
        }

        /* reset errno and rewait events if neccessary */
        errno = 0;
        if (mrb_isempty(buff) || (events & CAIO_OUT)) {
            CAIO_WAITFD(self, conn->fd, events);
        }
    }

    CAIO_FINALLY(self);
    if (conn->fd != -1) {
        caio_evloop_unregister(conn->fd);
        close(conn->fd);
    }
    if (mrb_destroy(conn->buff)) {
        ERROR("Cannot dispose buffers.");
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
        ERROR("Cannot bind on: "ADDRFMTS, ADDRFMTV(bindaddr));
        CAIO_RETURN(self);
    }

    /* Listen */
    res = listen(fd, backlog);
    INFO("Listening on: "ADDRFMTS" backlog: %d", ADDRFMTV(bindaddr), backlog);
    if (res) {
        ERROR("Cannot listen on: "ADDRFMTS, ADDRFMTV(bindaddr));
        CAIO_RETURN(self);
    }

    while (true) {
        connfd = accept4(fd, &connaddr, &addrlen, SOCK_NONBLOCK);
        if ((connfd == -1) && CAIO_MUSTWAITFD()) {
            CAIO_WAITFD(self, fd, CAIO_IN | CAIO_ET);
            continue;
        }

        if (connfd == -1) {
            ERROR("accept4");
            CAIO_RETURN(self);
        }

        /* New Connection */
        INFO("New connection from: "ADDRFMTS, ADDRFMTV(connaddr));
        struct tcpconn *c = malloc(sizeof(struct tcpconn));
        if (c == NULL) {
            ERROR("Out of memory");
            CAIO_RETURN(self);
        }

        c->fd = connfd;
        c->localaddr = bindaddr;
        c->remoteaddr = connaddr;
        c->buff = mrb_create(BUFFSIZE);
        if (tcpconn_spawn(echoA, c)) {
            ERROR("Maximum connection exceeded, fd: %d", connfd);
            close(connfd);
            mrb_destroy(c->buff);
            free(c);
        }
    }

    CAIO_FINALLY(self);
    if (fd != -1) {
        caio_evloop_unregister(fd);
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
