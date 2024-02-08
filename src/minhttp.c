#include "minhttp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/socket.h>
#include <poll.h>
#include <fcntl.h>

static void handle_request(int fd) {
}

/* entry point to the real server.
 * `socket' is a properly set up listen socket.
 * minhttp is not responsible for closing it. */
int minhttp(int socket) {
	int ready;
	nfds_t nfds = 1, i;
	nfds_t pfds_capacity = 8;
	struct pollfd *pfds = NULL;

	pfds = calloc(pfds_capacity, sizeof(struct pollfd));

	if (!pfds) {
		fprintf(stderr, "error: failed to allocate memory for polling. Are you out of memory?\n"
				" strerror(): %s\n", strerror(errno));
		return 1;
	}

	pfds[0].fd = socket;
	pfds[0].events = POLLIN;

	for (i = 1; i < pfds_capacity; i++) {
		pfds[i].fd = -1;
	}

	for (;;) {
		ready = poll(pfds, nfds, -1);

		if (pfds[0].revents & POLLIN) {
			int connection = accept(pfds[0].fd, NULL, NULL);

			if (connection < 0) {
				fprintf(stderr, "error: inbound connection failed: %s\n", strerror(errno));
				continue;
			}

			for (i = 1; i < nfds; i++) {
				if (pfds[i].fd == -1) {
					pfds[i].fd = connection;
					pfds[i].events = POLLIN;
					goto connection_done;
				}
			}

			if (nfds == pfds_capacity) {
				pfds_capacity = pfds_capacity * 3 / 2;
				pfds = realloc(pfds, pfds_capacity * sizeof(struct pollfd));

				for (i = nfds + 1; i < pfds_capacity; i++) {
					pfds[nfds].fd = -1;
					pfds[nfds].events = 0;
				}
			}

			pfds[nfds].fd = connection;
			pfds[nfds].events = POLLIN;

			nfds++;
		}

connection_done:

		for (i = 1; i < nfds; i++) {
			if (pfds[i].revents == 0) continue;

			if (pfds[i].revents & POLLERR || pfds[i].revents & POLLHUP) {
				close(pfds[i].fd);
				pfds[i].fd = -1;
				continue;
			}

			if (pfds[i].revents & POLLIN) {
				handle_request(pfds[i].fd);
			}
		}
	}

	free(pfds);

	return 0;
}
