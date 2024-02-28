#include "config.h"
#include "stringutil.h"

#include <log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <toml.h>

void init_server_config(struct server_config *cfg) {
	*cfg = (struct server_config) {
		.name = NULL,
		.host = NULL,
		.root = NULL,
		.address = "127.0.0.1",
		.http = {
			.enabled = 0,
			.port = 80,
		},
		.https = {
			.enabled = 0,
			.port = 443,
			.key = NULL,
		},
		.log = {
			.level = "WARN",
			.file = {
				.path = NULL,
				.level = "TRACE",
			},
		},
		.threads = -1,
		.backlog = 16,
	};
}

void free_config(struct config *cfg) {
	size_t i;

	for (i = 0; i < cfg->servers_count; i++)
		cleanup_server_config(&cfg->servers[i]);

	free(cfg->log.file.path);

	free(cfg);
}

void cleanup_server_config(struct server_config *cfg) {
	free(cfg->name);
	free(cfg->root);
	free(cfg->host);
	free(cfg->address);
	free(cfg->https.key);
	free(cfg->https.cert);
	free(cfg->log.level);
	free(cfg->log.file.path);
	free(cfg->log.file.level);
}

#define TABLE_REQUIRED(__table, __cfg, __elem, __T, __Ts,                      \
__errbuf, __errbuf_sz, __lbl)                                                  \
do {                                                                           \
	toml_datum_t __datum = toml_ ## __T ## _in(__table, #__elem);          \
	if (!__datum.ok) {                                                     \
		snprintf(__errbuf, __errbuf_sz, #__elem                        \
			" is a required field");                               \
		goto __lbl;                                                    \
	}                                                                      \
	__cfg->__elem = __datum.u.__Ts;                                        \
} while (0)

#define TABLE_OPTIONAL(__table, __cfg, __elem, __T, __Ts,                      \
__errbuf, __errbuf_sz, __lbl)                                                  \
do {                                                                           \
	if (toml_key_exists(__table, #__elem)) {                               \
		toml_datum_t __datum = toml_ ## __T ## _in(__table, #__elem);  \
		if (!__datum.ok) {                                             \
			snprintf(__errbuf, __errbuf_sz, #__elem                \
				" failed to parse");                           \
			goto __lbl;                                            \
		}                                                              \
		__cfg->__elem = __datum.u.__Ts;                                \
	}                                                                      \
} while (0)

#define TABLE_TABLE_OPTIONAL(__table, __cfg, __elem, __errbuf, __errbuf_sz,    \
	__func, __lbl)                                                         \
do {                                                                           \
	if (toml_key_exists(__table, #__elem)) {                               \
		toml_table_t *__tbl = toml_table_in(__table, #__elem);         \
		if (!__tbl) {                                                  \
			snprintf(__errbuf, __errbuf_sz, #__elem                \
				" failed to parse");                           \
			goto __lbl;                                            \
		}                                                              \
		__func(__tbl, &__cfg->__elem, __errbuf, __errbuf_sz);          \
	}                                                                      \
} while (0)

static int configure_http_from_toml(toml_table_t *table,
		struct config_http *cfg, char *errbuf, size_t errbuf_sz) {
	cfg->enabled = 1;

	TABLE_OPTIONAL(table, cfg, port, int, i, errbuf, errbuf_sz, cfg_err);

	return 0;

cfg_err:
	return 1;
}

static int configure_https_from_toml(toml_table_t *table,
		struct config_https *cfg, char *errbuf, size_t errbuf_sz) {
	cfg->enabled = 1;

	TABLE_OPTIONAL(table, cfg, port, int, i, errbuf, errbuf_sz, cfg_err);
	TABLE_REQUIRED(table, cfg, key, string, s, errbuf, errbuf_sz, cfg_err);
	TABLE_REQUIRED(table, cfg, cert, string, s, errbuf, errbuf_sz, cfg_err);

	return 0;

cfg_err:
	return 1;
}

static int configure_log_file_from_toml(toml_table_t *table,
		struct config_log_file *cfg, char *errbuf, size_t errbuf_sz) {

	TABLE_REQUIRED(table, cfg, path, string, s, errbuf, errbuf_sz,
		cfg_err);
	TABLE_OPTIONAL(table, cfg, level, string, s, errbuf, errbuf_sz,
		cfg_err);

	return 0;

cfg_err:
	return 1;
}

static int configure_log_from_toml(toml_table_t *table,
		struct config_log *cfg, char *errbuf, size_t errbuf_sz) {

	TABLE_OPTIONAL(table, cfg, level, string, s, errbuf, errbuf_sz,
		cfg_err);

	TABLE_TABLE_OPTIONAL(table, cfg, file, errbuf, errbuf_sz,
		configure_log_file_from_toml, cfg_err);

	return 0;

cfg_err:
	return 1;
}

static int create_server_config_from_toml(toml_table_t *table,
		struct server_config *cfg, char *errbuf, size_t errbuf_sz) {
	init_server_config(cfg);

	TABLE_REQUIRED(table, cfg, name, string, s, errbuf, errbuf_sz, cfg_err);
	TABLE_REQUIRED(table, cfg, root, string, s, errbuf, errbuf_sz, cfg_err);
	TABLE_REQUIRED(table, cfg, host, string, s, errbuf, errbuf_sz, cfg_err);
	TABLE_OPTIONAL(table, cfg, address, string, s, errbuf, errbuf_sz,
		cfg_err);
	TABLE_OPTIONAL(table, cfg, threads, int, i, errbuf, errbuf_sz, cfg_err);
	TABLE_OPTIONAL(table, cfg, backlog, int, i, errbuf, errbuf_sz, cfg_err);

	TABLE_TABLE_OPTIONAL(table, cfg, http, errbuf, errbuf_sz,
		configure_http_from_toml, cfg_err);
	TABLE_TABLE_OPTIONAL(table, cfg, https, errbuf, errbuf_sz,
		configure_https_from_toml, cfg_err);
	TABLE_TABLE_OPTIONAL(table, cfg, log, errbuf, errbuf_sz,
		configure_log_from_toml, cfg_err);

	return 0;

cfg_err:
	cleanup_server_config(cfg);
	return 1;
}

struct config *parse_config(const char *config_file_path,
			    char *errbuf, size_t errbuf_sz) {
	FILE *f = NULL;
	toml_table_t *toml = NULL;
	toml_array_t *servers = NULL;
	size_t i;
	struct config *cfg = NULL;

	f = fopen(config_file_path, "r");

	if (!f) {
		snprintf(errbuf, errbuf_sz, "failed to open config file '%s'",
			config_file_path);
		return NULL;
	}

	toml = toml_parse_file(f, errbuf, errbuf_sz);

	fclose(f);

	if (!toml)
		return NULL;

	servers = toml_array_in(toml, "server");

	if (!servers) {
		snprintf(errbuf, errbuf_sz,
			"config is required to have at least one"
			" server definition");
		toml_free(toml);
		return NULL;
	}

	cfg = malloc(sizeof(struct config));

	if (!cfg) {
		snprintf(errbuf, errbuf_sz, "out of memory");
		toml_free(toml);
		return NULL;
	}

	cfg->log = (struct config_log) {
		.level = "WARN",
		.file = {
			.level = "INFO",
			.path = NULL,
		},
	};

	cfg->servers_count = toml_array_nelem(servers);
	cfg->servers = malloc(sizeof(struct server_config) * cfg->servers_count);

	if (!cfg->servers) {
		snprintf(errbuf, errbuf_sz, "out of memory");
		toml_free(toml);
		free(cfg);
		return NULL;
	}

	for (i = 0; i < cfg->servers_count; i++) {
		toml_table_t *server = toml_table_at(servers, i);

		if (create_server_config_from_toml(
				server, &cfg->servers[i], errbuf, errbuf_sz)) {
			toml_free(toml);
			free(cfg->servers);
			free(cfg);
			return NULL;
		}
	}

	configure_log_from_toml(toml, &cfg->log, errbuf, errbuf_sz);

	toml_free(toml);

	return cfg;
}
