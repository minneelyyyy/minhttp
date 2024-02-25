#include "config.h"
#include "stringutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <toml.h>

void init_config(struct config *cfg) {
	*cfg = (struct config) {
		.name = NULL,
		.port = 80,
		.root = NULL,
		.host = NULL,
		.logfile = NULL,
		.backlog = 16,
		.keyfile = NULL,
		.certfile = NULL,
		.threads = -1,
	};
}

void free_configs(struct config *cfg, size_t count) {
	size_t i;

	for (i = 0; i < count; i++)
		cleanup_config(&cfg[i]);

	free(cfg);
}

void cleanup_config(struct config *cfg) {
	free(cfg->name);
	free(cfg->root);
	free(cfg->host);
	free(cfg->logfile);
	free(cfg->keyfile);
	free(cfg->certfile);
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

static int create_config_from_toml(toml_table_t *table, struct config *cfg,
		char *errbuf, size_t errbuf_sz) {
	toml_datum_t datum;

	init_config(cfg);

	TABLE_REQUIRED(table, cfg, name, string, s, errbuf, errbuf_sz, cfg_err);
	TABLE_OPTIONAL(table, cfg, port, int, i, errbuf, errbuf_sz, cfg_err);
	TABLE_REQUIRED(table, cfg, root, string, s, errbuf, errbuf_sz, cfg_err);
	TABLE_REQUIRED(table, cfg, host, string, s, errbuf, errbuf_sz, cfg_err);
	TABLE_OPTIONAL(table, cfg, logfile, string, s, errbuf, errbuf_sz,
		cfg_err);
	TABLE_OPTIONAL(table, cfg, backlog, int, i, errbuf, errbuf_sz, cfg_err);
	TABLE_OPTIONAL(table, cfg, keyfile, string, s, errbuf, errbuf_sz,
		cfg_err);
	TABLE_OPTIONAL(table, cfg, certfile, string, s, errbuf, errbuf_sz,
		cfg_err);
	TABLE_OPTIONAL(table, cfg, threads, int, i, errbuf, errbuf_sz,
		cfg_err);

	return 0;

cfg_err:
	cleanup_config(cfg);
	return 1;
}

struct config *parse_config(const char *config_file_path, size_t *count,
			    char *errbuf, size_t errbuf_sz) {
	FILE *f = NULL;
	toml_table_t *toml = NULL;
	toml_array_t *servers = NULL;
	size_t i;
	struct config *cfgs = NULL;

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

	*count = toml_array_nelem(servers);
	cfgs = malloc(sizeof(struct config) * *count);

	if (!cfgs) {
		snprintf(errbuf, errbuf_sz, "out of memory");
		toml_free(toml);
		return NULL;
	}

	for (i = 0; i < *count; i++) {
		toml_table_t *server = toml_table_at(servers, i);

		if (create_config_from_toml(
				server, &cfgs[i], errbuf, errbuf_sz)) {
			toml_free(toml);
			free(cfgs);
			return NULL;
		}
	}

	toml_free(toml);

	return cfgs;
}
