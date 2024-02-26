#include "proxy.h"
#include "server.h"

#include <log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

struct server_manage_info {
	struct server srv;
};

struct socket_info {
	struct sockaddr_in addr;
	int fd;
};

static int create_http_socket(struct config *cfg) {
	int fd;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);

	if (!fd) 
		return 1;

	if (listen(fd, cfg->backlog))
		return 1;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(cfg->http.port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(fd, (const struct sockaddr*) &addr, sizeof(addr)))
		return 1;

	return fd;
}

static int proxy_poll(struct server_manage_info *servers, size_t server_cnt,
		struct socket_info *sockets, size_t socket_cnt) {
	nfds_t cfds = socket_cnt;
	nfds_t nfds = socket_cnt;
	size_t clients = 0;
	struct pollfd *pfds = malloc(sizeof(struct pollfd) * socket_cnt);
	size_t i;

	for (i = 0; i < socket_cnt; i++) {
		pfds[i].fd = sockets[i].fd;
		pfds[i].events = POLLIN | POLLOUT;
	}

	for (;;) {
		int ready = poll(pfds, nfds, -1);

		/* sockets accept connections on */
		for (i = 0; i < socket_cnt; i++) {
		}

		/* clients connected directly to the proxy */
		for (i = socket_cnt; i < socket_cnt + clients * 2; i += 2) {
		}
	}

	free(pfds);

	return 0;
}

static int port_already_exists(struct socket_info *sockets, size_t socket_cnt,
		unsigned short port) {
	size_t i;

	for (i = 0; i < socket_cnt; i++) {
		if (sockets[i].addr.sin_port == htons(port))
			return 1;
	}

	return 0;
}

int minhttp_proxy(struct config *server_cfgs, size_t server_cnt) {
	size_t i;
	int err = 0;
	struct server_manage_info *servers =
		malloc(sizeof(struct server_manage_info) * server_cnt);

	/* the maximum number of sockets possible is 1* for each server.
	 * it can also be less, but it isn't worth saving the few bytes
	 * this struct costs to store for the downside of making this more
	 * complicated.
	 * 
	 * *1 because the server doesnt support https yet. there will
	 *   eventually be a need for more sockets per server. */
	struct socket_info *sockets =
		malloc(sizeof(struct socket_info) * server_cnt);
	size_t socket_cnt = 0;

	if (!servers || !sockets) {
		log_error("out of memory");
		return 1;
	}

	/* setup child procs first. this gives them time to do their setup
	 * without waiting for the proxy server to setup. */
	for (i = 0; i < server_cnt; i++) {
		struct config *cfg = &server_cfgs[i];
		unsigned short fake_port = 8000 + i;

		pid_t pid;
		int fd;

		servers[i].srv.cfg = cfg;

		servers[i].srv.addr.sin_family = AF_INET;
		servers[i].srv.addr.sin_port = htons(fake_port);
		inet_aton(cfg->address, &servers[i].srv.addr.sin_addr);

		pid = fork();

		if (pid < 0) {
			log_error("failed to fork process: %s\n",
				strerror(errno));

			err = 1;
			goto cleanup;
		}

		if (!pid) {
			err = server_entry(&servers[i].srv);
			goto cleanup;
		}
	}

	/* setup the proxy server */
	for (i = 0; i < server_cnt; i++) {
		struct config *cfg = &server_cfgs[i];

		/* sets up a net-facing socket for the proxy for http */
		if (cfg->http.enabled && !port_already_exists(sockets,
						socket_cnt, cfg->http.port)) {
			sockets[socket_cnt].fd = create_http_socket(cfg);

			if (!sockets[socket_cnt].fd) {
				log_error("failed to open socket: %s",
					strerror(errno));

				err = 1;
				goto cleanup;
			}

			sockets[socket_cnt].addr = servers[i].srv.addr;
			socket_cnt++;
		}

		/* same thing for https */
		if (cfg->https.enabled) {
			log_fatal("https is not yet supported");
			err = 1;
			goto cleanup;
		}
	}

	proxy_poll(servers, server_cnt, sockets, socket_cnt);

	for (i = 0; i < server_cnt; i++) {
		close(sockets[i].fd);
	}

cleanup:
	free(servers);
	free(sockets);

	return err;
}
