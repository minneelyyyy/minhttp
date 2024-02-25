#ifndef __MINHTTP_CONFIG_H
#define __MINHTTP_CONFIG_H

#include <stddef.h>

struct config {
	/* the name of the server */
	char *name;

	/* the port the server runs on */
	unsigned short port;

	/* the root directory of the web server, where it serves files from */
	char *root;

	/* the host name of the server (eg. www.example.com) */
	char *host;

	/* file to write log messages to */
	char *logfile;

	/* maximum number of clients awaiting connection */
	int backlog;

	/* key file used for encryption */
	char *keyfile;

	/* cert file used for encryption */
	char *certfile;

	/* worker thread count */ 
	int threads;
};

void init_config(struct config *cfg);

void cleanup_config(struct config *cfg);

void free_configs(struct config *cfgs, size_t count);

/** parse a config file written in toml
 * @param config_file_path the file to parse
 * @param count writes the number of configs to this argument (should be the number of servers)
 * @param errbuf buffer to write an error string to
 * @param errbuf_sz size of errbuf in bytes
 * @return a pointer to an array of config structs */
struct config *parse_config(const char *config_file_path, size_t *count, char *errbuf, size_t errbuf_sz);

#endif /* __MINHTTP_CONFIG_H */
