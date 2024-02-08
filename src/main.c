#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "minhttp.h"

#include <config.h>

int main(void) {
	int fd;
	int opt = 1;
	int ret;
	struct sockaddr_in address;

	fd = socket(AF_INET, SOCK_STREAM, 0);

	if (fd < 0) {
		fprintf(stderr, "error: failed to create socket: %s\n", strerror(errno));
		return 1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		fprintf(stderr, "error: failed to set socket options: %s\n", strerror(errno));
		return 1;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
		fprintf(stderr, "error: failed to bind socket (%d): %s\n", fd, strerror(errno));
		return 1;
	}

	if (listen(fd, BACKLOG) < 0) {
		fprintf(stderr, "error: failed to listen on socket (%d): %s\n", fd, strerror(errno));
		return 1;
	}

	ret = minhttp(fd);

	close(fd);

	return ret;
}
