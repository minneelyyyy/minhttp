#define _XOPEN_SOURCE 500

#include <config.h>
#include "proxy.h"

#include <log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

static void daemonize(void) {
	pid_t pid = fork();

	if (pid < 0) {
		log_error("failed to daemonize: %s", strerror(errno));
		exit(1);
	}

	if (pid)
		exit(0);
}

int main(int argc, char **argv) {
	char errbuf[512];
	char *config_path = NULL;
	struct config *cfg = NULL;
	int c, ret;
	int is_daemon = 0;

	while ((c = getopt(argc, argv, "dC:")) != -1) {
		switch (c) {
			case 'C':
				if (!config_path)
					config_path = strdup(optarg);
				break;
			case 'd':
				if (!is_daemon) {
					daemonize();
					is_daemon = 1;
				}
				break;
		}
	}

	cfg = parse_config(
		config_path ? config_path : "config.toml",
		errbuf, sizeof(errbuf));

	free(config_path);

	if (!cfg) {
		log_error("failed to parse config: %s", errbuf);
		return 1;
	}

	ret = minhttp_proxy(cfg);

	free_config(cfg);

	return ret;
}
