#define _XOPEN_SOURCE 500

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

extern int server_start(struct config *cfg, int socket);

static void daemonize(void) {
	pid_t pid = fork();

	if (pid < 0) {
		fprintf(stderr, "error: failed to daemonize: %s\n",
			strerror(errno));
		exit(1);
	}

	if (pid)
		exit(0);
}

int main(int argc, char **argv) {
	char errbuf[512];
	char *config_path = NULL;
	struct config *configs = NULL;
	size_t config_nr;
	pid_t pid;
	int c;
	int is_daemon = 0;

	while ((c = getopt(argc, argv, "dC:")) != -1) {
		switch (c) {
			case 'C':
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

	configs = parse_config(
		config_path ? config_path : "config.toml",
		&config_nr, errbuf, sizeof(errbuf));

	free(config_path);

	if (!configs) {
		fprintf(stderr, "error: failed to parse config: %s\n", errbuf);
		return 1;
	}

	for (size_t i = 0; i < config_nr; i++) {
		struct config *cfg = &configs[i];

		pid = fork();

		if (pid == -1) {
			fprintf(stderr, "error: failed to fork process: %s\n",
				strerror(errno));
			return 1;
		}

		if (!pid)
			return server_start(cfg);
	}

	free_configs(configs, config_nr);

	return 0;
}
