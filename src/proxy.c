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
	int httpfd;
};

static int create_http_socket(struct config *cfg) {
	int fd;
	int opt = 1;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);

	if (!fd) 
		return 1;

	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

	if (listen(fd, cfg->backlog))
		return 1;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(cfg->http.port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(fd, (const struct sockaddr*) &addr, sizeof(addr)))
		return 1;

	return fd;
}

static int proxy_poll(struct server_manage_info *servers, size_t server_cnt) {
	return 0;
}

int minhttp_proxy(struct config *server_cfgs, size_t server_cnt) {
	size_t i, client_cnt = 0;
	struct server_manage_info *servers =
		malloc(sizeof(struct server_manage_info) * server_cnt);

	if (!servers) {
		log_error("out of memory");
		return 1;
	}

	for (i = 0; i < server_cnt; i++) {
		struct config *cfg = &server_cfgs[i];
		unsigned short fake_port = 8000 + i;

		pid_t pid;
		int fd;

		servers[i].srv.cfg = cfg;

		fd = socket(AF_INET, SOCK_STREAM, 0);

		if (fd < 0) {
			log_error("failed to open socket: %s", strerror(errno));
			free(servers);
			return 1;
		}

		servers[i].srv.socket = fd;

		servers[i].srv.addr.sin_family = AF_INET;
		servers[i].srv.addr.sin_port = htons(fake_port);
		inet_aton(cfg->address, &servers[i].srv.addr.sin_addr);

		servers[i].httpfd = -1;

		if (cfg->http.enabled) {
			servers[i].httpfd = create_http_socket(cfg);

			if (!servers[i].httpfd) {
				log_error("failed to open socket: %s",
					strerror(errno));
				free(servers);
				return 1;
			}
		}

		if (cfg->https.enabled) {
			log_fatal("https is not yet supported");
			free(servers);
			return 1;
		}

		pid = fork();

		if (pid < 0) {
			log_error("failed to fork process: %s\n",
				strerror(errno));

			free(servers);
			return 1;
		}

		if (!pid) {
			int ret = server_entry(&servers[i].srv);
			free(servers);
			return ret;
		}
	}

	free(servers);
	return 0;
}
