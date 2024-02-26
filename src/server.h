#ifndef __MINHTTP_SERVER_H
#define __MINHTTP_SERVER_H

#include <netinet/in.h>

struct server {
	struct config *cfg;
	struct sockaddr_in addr;
	int socket;
};

int server_entry(struct server *srv);

#endif /* __MINHTTP_SERVER_H */
