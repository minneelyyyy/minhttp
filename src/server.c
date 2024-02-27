#include <config.h>
#include "server.h"

#include <log.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

int server_entry(struct server *srv) {
	FILE *logfile = NULL;
	int opt = 1;
	int sock;
	int level;

	level = log_level_from_string(srv->cfg->log.level);

	if (level == -1) {
		log_warn("failed to get log level, setting default WARN");
		level = LOG_WARN;
	}

	log_set_level(level);

	if (srv->cfg->log.file.path) {
		logfile = fopen(srv->cfg->log.file.path, "w");

		if (!logfile) {
			log_warn("failed to open log file '%s' for writing",
				srv->cfg->log.file.path);
			fclose(logfile);
			logfile = NULL;
		} else {
			level = log_level_from_string(
				srv->cfg->log.file.level);

			if (level == -1) {
				log_warn( "failed to get log level"
					" for file, setting INFO");
				level = LOG_INFO;
			}

			log_add_fp(logfile, level);
		}
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock < 0) {
		log_error("failed to create socket: %s", strerror(errno));
		return 1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		log_error("failed to set socket option REUSEADDR: %s", strerror(errno));
		return 1;
	}

	if (bind(sock, (struct sockaddr*) &srv->addr,
				sizeof(srv->addr))) {
		log_error("failed to bind: %s", strerror(errno));
		return 1;
	}

	if (listen(sock, srv->cfg->backlog)) {
		log_error("failed to listen on socket: %s", strerror(errno));
		return 1;
	}

	for (;;) {
		int fd = accept(sock, NULL, NULL);
		char buffer[512];
		ssize_t r;

		while ((r = recv(fd, buffer, sizeof(buffer), 0)) > 0) {
			send(fd, buffer, r, 0);
		}

		close(fd);
	}

	/* DANGEROUS: at this point, the logging library still holds a pointer
	 * to this file, but we also don't want to keep it open forever.
	 * log.c doesn't provide a way of removing a file.
	 * remember not to log anything after closing this file. */
	if (logfile)
		fclose(logfile);

	return 0;
}
