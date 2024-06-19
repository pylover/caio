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
#include <netinet/in.h>
#include <sys/socket.h>
// #include <fcntl.h>
// #include <stdint.h>
// #include <arpa/inet.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/time.h>
// #include <unistd.h>


#define TCP_BACKLOG 1024


static int
setup_listening_socket(int port) {
	struct sockaddr_in srv_addr = { };
	int fd;
    int enable;
    int ret;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("socket()");
		return -1;
	}

	enable = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	if (ret < 0) {
		perror("setsockopt(SO_REUSEADDR)");
		return -1;
	}

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(fd, (const struct sockaddr *)&srv_addr, sizeof(srv_addr));

	if (ret < 0) {
		perror("bind()");
		return -1;
	}

	if (listen(fd, TCP_BACKLOG) < 0) {
		perror("listen()");
		return -1;
	}

	return fd;
}


int
main() {
    return 0;
}
