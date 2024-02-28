#include "proxy.h"
#include "server.h"

#include <log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>
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

struct client_manage_info {
	struct socket_info sock;
	char buffer[8096];
	size_t buffer_size, buffer_off;
};

static struct client_manage_info *get_client(
		struct client_manage_info *cl, size_t cls, int fd) {
	size_t i;

	for (i = 0; i < cls; i++) {
		if (cl[i].sock.fd == fd)
			return &cl[i];
	}

	return NULL;
}

static struct client_manage_info *get_unused_client(
		struct client_manage_info *clients, size_t count) {
	size_t i;

	for (i = 0; i < count; i++)
		if (clients[i].sock.fd == -1)
			return &clients[i];

	return &clients[count];
}

static void disconnect_client(struct client_manage_info *client,
		struct pollfd *pfd) {
	char saddr[INET_ADDRSTRLEN];

	close(pfd->fd);

	pfd->fd = -1;
	client->sock.fd = -1;

	inet_ntop(AF_INET, &client->sock.addr.sin_addr, saddr,
		INET_ADDRSTRLEN);

	log_info("%s disconnected", saddr);
}

/* NOTE: NOT THREAD SAFE! Will need changes if we make the proxy MT!
 * particularly, the static elements create a race condition. */
static int client_manager(struct server_manage_info *srv,
		size_t srv_cnt, struct pollfd *pfd) {
	static struct client_manage_info *clients = NULL;
	static size_t clients_size = 0;
	static size_t clients_capacity = 0;

	struct client_manage_info *client;
	size_t i;

	/* NOTE: this code assumes we only ever add one client at a 
	 * time. if that changes, this code needs to change too. */
	if (clients_size + 1 >= clients_capacity) {
		clients_capacity = clients_capacity > 0
			? clients_capacity * 2 : 8;

		clients = realloc(clients,
		    sizeof(struct client_manage_info) * clients_capacity);

		for (i = clients_size; i < clients_capacity; i++) {
			clients[i].sock.fd = -1;
			clients[i].buffer_size = 0;
			clients[i].buffer_off = 0;
		}
	}

	client = get_client(clients, clients_size, pfd->fd);

	/* this is a client we have not seen before */
	if (!client) {
		socklen_t len = sizeof(struct sockaddr_in);
		client = get_unused_client(clients, clients_size);

		client->sock.fd = pfd->fd;
		getsockname(pfd->fd, (struct sockaddr*) &client->sock.addr,
			&len);

		clients_size++;
	}

	if (pfd->revents & POLLHUP) {
		disconnect_client(client, pfd);
		return 0;
	}

	if ((pfd->revents & POLLIN)
			&& client->buffer_size < sizeof(client->buffer)) {
		ssize_t c;

		c = recv(pfd->fd, client->buffer + client->buffer_size,
			sizeof(client->buffer) - client->buffer_size
				- client->buffer_off, 0);

		if (c < 1) {
			disconnect_client(client, pfd);
			return 0;
		}

		client->buffer_size += c;
	}

	if ((pfd->revents & POLLOUT) && client->buffer_size > 0) {
		ssize_t r;
		size_t amt = client->buffer_size - client->buffer_off;

		r = send(pfd->fd, client->buffer + client->buffer_off, amt, 0);

		if (r < 1) {
			log_warn("send returned <1");
			return 0;
		} else if (r < amt) {
			client->buffer_off += r;
		} else {
			client->buffer_off = 0;
			client->buffer_size = 0;
		}
	}

	return 0;
}

static int accept_connection(int sock, struct pollfd **pfds, nfds_t *nfds,
		nfds_t *cfds) {
	size_t i;
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	char saddr[INET_ADDRSTRLEN];

	int con = accept(sock, (struct sockaddr*) &addr, &len);

	if (con < 0) {
		log_warn("failed to accept new connection");
		return 0;
	}

	inet_ntop(AF_INET, &addr.sin_addr, saddr, INET_ADDRSTRLEN);
	log_info("connection from %s:%hu", saddr, htons(addr.sin_port));

	/* try to find an unused element in pfds */
	for (i = 0; i < *nfds; i++) {
		if ((*pfds)[i].fd < 0) {
			log_debug("reusing pfd %lu", i);
			(*pfds)[i].fd = con;
			return 0;
		}
	}

	/* resize if necessary */
	if (*nfds == *cfds) {
		struct pollfd *new_pfds;
		nfds_t new_cfds = *cfds * 2;

		log_debug("resizing pfds from %lu to %lu", *cfds, new_cfds);

		new_pfds = realloc(*pfds, sizeof(struct pollfd) * new_cfds);

		if (new_pfds == NULL) {
			log_warn("failed to allocate space for new connection");
			close(con);
			return 0;
		}

		*cfds = new_cfds;
		*pfds = new_pfds;
	}

	(*pfds)[*nfds].fd = con;
	(*pfds)[*nfds].events = POLLIN | POLLOUT;
	(*pfds)[*nfds].revents = 0;

	(*nfds)++;

	log_debug("appended pollfd at end. nfds = %lu", *nfds);

	return 0;
}

static int proxy_poll(struct server_manage_info *servers, size_t server_cnt,
		struct socket_info *sockets, size_t socket_cnt) {
	nfds_t cfds = socket_cnt * 2;
	nfds_t nfds = socket_cnt;
	struct pollfd *pfds = malloc(sizeof(struct pollfd) * cfds);
	size_t i;
	int ret = 0;

	for (i = 0; i < socket_cnt; i++) {
		pfds[i].fd = sockets[i].fd;
		pfds[i].events = POLLIN;
	}

	log_info("ready to accept connections on proxy");

	for (;;) {
		int ready = poll(pfds, nfds, -1);

		/* clients connected directly to the proxy */
		for (i = socket_cnt; i < nfds; i++) {
			ret = client_manager(servers, server_cnt, &pfds[i]);

			if (ret)
				goto err;
		}

		/* sockets to accept connections on */
		for (i = 0; i < socket_cnt; i++) {
			if (pfds[i].revents & POLLIN) {
				ret = accept_connection(pfds[i].fd,
					&pfds, &nfds, &cfds);

				if (ret)
					goto err;
			}
		}
	}

err:
	free(pfds);
	return ret;
}

static int create_http_socket(struct server_config *cfg) {
	int fd;
	struct sockaddr_in addr;
	int opt = 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);

	if (!fd) 
		return 1;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(cfg->http.port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(fd, (const struct sockaddr*) &addr, sizeof(addr)))
		return 1;

	if (listen(fd, cfg->backlog))
		return 1;

	return fd;
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

int minhttp_proxy(struct config *cfg) {
	size_t i;
	int err = 0;
	FILE *logfile = NULL;
	int level;
	struct server_manage_info *servers =
		malloc(sizeof(struct server_manage_info) * cfg->servers_count);

	level = log_level_from_string(cfg->log.level);

	if (level == -1) {
		log_warn("failed to get log level, setting default WARN");
		level = LOG_WARN;
	}

	log_set_level(level);

	if (cfg->log.file.path) {
		logfile = fopen(cfg->log.file.path, "w");

		if (!logfile) {
			log_warn("failed to open log file '%s' for writing",
				cfg->log.file.path);
			fclose(logfile);
			logfile = NULL;
		} else {
			level = log_level_from_string(
				cfg->log.file.level);

			if (level == -1) {
				log_warn( "failed to get log level"
					" for file, setting INFO");
				level = LOG_INFO;
			}

			log_add_fp(logfile, level);
		}
	}

	/* the maximum number of sockets possible is 1* for each server.
	 * it can also be less, but it isn't worth saving the few bytes
	 * this struct costs to store for the downside of making this more
	 * complicated.
	 * 
	 * *1 because the server doesnt support https yet. there will
	 *   eventually be a need for more sockets per server. */
	struct socket_info *sockets =
		malloc(sizeof(struct socket_info) * cfg->servers_count);
	size_t socket_cnt = 0;

	if (!servers || !sockets) {
		log_error("out of memory");
		return 1;
	}

	/* setup child procs first. this gives them time to do their setup
	 * without waiting for the proxy server to setup. */
	for (i = 0; i < cfg->servers_count; i++) {
		struct server_config *scfg = &cfg->servers[i];
		unsigned short fake_port = 8000 + i;

		pid_t pid;
		int fd;

		servers[i].srv.cfg = scfg;

		servers[i].srv.addr.sin_family = AF_INET;
		servers[i].srv.addr.sin_port = htons(fake_port);
		inet_aton(scfg->address, &servers[i].srv.addr.sin_addr);

		pid = fork();

		if (pid < 0) {
			log_error("failed to fork process: %s\n",
				strerror(errno));

			err = 1;
			goto cleanup;
		}

		if (!pid) {
			prctl(PR_SET_PDEATHSIG, SIGHUP);
			err = server_entry(&servers[i].srv);
			goto cleanup;
		}
	}

	/* setup the proxy server */
	for (i = 0; i < cfg->servers_count; i++) {
		struct server_config *scfg = &cfg->servers[i];

		/* sets up a net-facing socket for the proxy for http */
		if (scfg->http.enabled && !port_already_exists(sockets,
						socket_cnt, scfg->http.port)) {
			sockets[socket_cnt].fd = create_http_socket(scfg);

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
		if (scfg->https.enabled) {
			log_fatal("https is not yet supported");
			err = 1;
			goto cleanup;
		}
	}

	proxy_poll(servers, cfg->servers_count, sockets, socket_cnt);

	for (i = 0; i < cfg->servers_count; i++) {
		close(sockets[i].fd);
	}

cleanup:
	free(servers);
	free(sockets);

	if (logfile)
		fclose(logfile);

	return err;
}
