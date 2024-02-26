#include <config.h>
#include "server.h"

#include <log.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

int server_entry(struct server *srv) {
	FILE *logfile = NULL;

	if (srv->cfg->logfile) {
		logfile = fopen(srv->cfg->logfile, "w");

		if (!logfile) {
			log_warn("failed to open log file '%s' for writing",
				srv->cfg->logfile);
		} else {
			log_add_fp(logfile, LOG_INFO);
		}
	}

	if (bind(srv->socket, (struct sockaddr*) &srv->addr,
				sizeof(srv->addr))) {
		log_error("failed to bind");
		return 1;
	}

	if (listen(srv->socket, srv->cfg->backlog)) {
		log_error("failed to listen on socket");
		return 1;
	}

	/* ... */

	/* DANGEROUS: at this point, the logging library still holds a pointer
	 * to this file, but we also don't want to keep it open forever.
	 * log.c doesn't provide a way of removing a file.
	 * remember not to log anything after closing this file. */
	if (logfile)
		fclose(logfile);

	return 0;
}
