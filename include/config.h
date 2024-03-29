#ifndef __MINHTTP_CONFIG_H
#define __MINHTTP_CONFIG_H

#include <stddef.h>

struct server_config {
	/** the server's name */
	char *name;

	/** the host address of the server */
	char *host;

	/** the root filesystem of the server */
	char *root;

	/** the local address to listen on */
	char *address;

	/** http options */
	struct config_http {
		/** 1 if http is enabled */
		int enabled;

		/** port to accept http connections on */
		unsigned short port;
	} http;

	/** https options */
	struct config_https {
		/** 1 if https is enabled */
		int enabled;

		/** port to accept https connections on */
		unsigned short port;

		/** filepath to the server's TLS key */
		char *key;

		/** filepath to the server's TLS certificate */
		char *cert;
	} https;

	/** logging options */
	struct config_log {
		/** level to log to stdout at */
		char *level;

		/** file to log to */
		struct config_log_file {
			/** filepath of the file */
			char *path;

			/** level to log to log file at */
			char *level;
		} file;
	} log;

	/** number of worker threads */
	int threads;

	/** number of connections to let pend */
	int backlog;
};

struct config {
	struct config_log log;
	struct server_config *servers;
	size_t servers_count;
};

void init_server_config(struct server_config *cfg);

void cleanup_server_config(struct server_config *cfg);

void free_config(struct config *cfg);

/** parse a config file written in toml
 * @param config_file_path the file to parse
 * @param errbuf buffer to write an error string to
 * @param errbuf_sz size of errbuf in bytes
 * @return a pointer to an array of config structs */
struct config *parse_config(const char *config_file_path, char *errbuf, size_t errbuf_sz);

#endif /* __MINHTTP_CONFIG_H */
