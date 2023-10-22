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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <mrb.h>

#include "caio.h"


/* TCP server caio state and */
struct tcpserver {
    struct sockaddr_in bindaddr;
    int backlog;
};


/* TCP connection state types */
struct tcpconn {
    int fd;
    struct sockaddr_in localaddr;
    struct sockaddr_in remoteaddr;
    mrb_t buff;
};


#define PAGESIZE 4096
#define BUFFSIZE (PAGESIZE * 32768)


#define ADDRFMTS "%s:%d"
#define ADDRFMTV(a) inet_ntoa((a).sin_addr), ntohs((a).sin_port)



static ASYNC
echoA(struct caio_task *self, struct tcpconn *conn) {
    ssize_t bytes;
    struct mrb *buff = conn->buff;
    CORO_START;
    static int events = 0;

    while (true) {
        events = CAIO_ET;

        /* tcp write */
        /* Write as mush as possible until EAGAIN */
        while (!mrb_isempty(buff)) {
            bytes = mrb_writeout(buff, conn->fd, mrb_used(buff));
            DEBUG("writing: %d bytes: %d", conn->fd, bytes);
            if ((bytes == -1) && CORO_MUSTWAITFD()) {
                events |= CAIO_OUT;
                break;
            }
            if (bytes == -1) {
                CORO_REJECT("write(%d)", conn->fd);
            }
            if (bytes == 0) {
                CORO_REJECT("write(%d) EOF", conn->fd);
            }
        }

        /* tcp read */
        /* Read as mush as possible until EAGAIN */
        while (!mrb_isfull(buff)) {
            bytes = mrb_readin(buff, conn->fd, mrb_available(buff));
            DEBUG("reading: %d bytes: %d", conn->fd, bytes);
            if ((bytes == -1) && CORO_MUSTWAITFD()) {
                events |= CAIO_IN;
                break;
            }
            if (bytes == -1) {
                CORO_REJECT("read(%d)", conn->fd);
            }
            if (bytes == 0) {
                CORO_REJECT("read(%d) EOF", conn->fd);
            }
        }

        /* reset errno and rewait events if neccessary */
        errno = 0;
        if (mrb_isempty(buff) || (events & CAIO_OUT)) {
            CORO_WAITFD(conn->fd, events);
        }
    }

    CORO_FINALLY;
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
tcpserverA(struct caio_task *self, struct tcpserver *state) {
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr_in connaddr;
    static int fd;
    int connfd;
    int res;
    int option = 1;
    CORO_START;

    /* Create socket */
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    /* Allow reuse the address */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* Bind to tcp port */
    res = bind(fd, &state->bindaddr, sizeof(state->bindaddr));
    if (res) {
        CORO_REJECT("Cannot bind on: "ADDRFMTS, ADDRFMTV(state->bindaddr));
    }

    /* Listen */
    res = listen(fd, state->backlog);
    INFO("Listening on: "ADDRFMTS, ADDRFMTV(state->bindaddr));
    if (res) {
        CORO_REJECT("Cannot listen on: "ADDRFMTS, ADDRFMTV(state->bindaddr));
    }

    while (true) {
        connfd = accept4(fd, &connaddr, &addrlen, SOCK_NONBLOCK);
        if ((connfd == -1) && CORO_MUSTWAITFD()) {
            CORO_WAITFD(fd, CAIO_IN | CAIO_ET);
            continue;
        }

        if (connfd == -1) {
            CORO_REJECT("accept4");
        }

        /* New Connection */
        INFO("New connection from: "ADDRFMTS, ADDRFMTV(connaddr));
        struct tcpconn *c = malloc(sizeof(struct tcpconn));
        if (c == NULL) {
            CORO_REJECT("Out of memory");
        }

        c->fd = connfd;
        c->localaddr = state->bindaddr;
        c->remoteaddr = connaddr;
        c->buff = mrb_create(BUFFSIZE);
        if (CAIO_RUN(echoA, c)) {
            ERROR("Maximum connection exceeded, fd: %d", connfd);
            close(connfd);
            mrb_destroy(c->buff);
            free(c);
        }
    }

    CORO_FINALLY;
    if (fd != -1) {
        caio_evloop_unregister(fd);
        close(fd);
    }
}


int
main() {
    struct tcpserver state = {
        .bindaddr = {
            .sin_addr = {htons(0)},
            .sin_port = htons(3030),
        },
        .backlog = 2,
    };

    return CAIO(tcpserverA, &state, 5);
}
