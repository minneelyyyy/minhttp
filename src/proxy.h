#ifndef __MINHTTP_H
#define __MINHTTP_H

#include <config.h>

#include <stddef.h>

/** entry point to the proxy server that manages all servers, post configuration parsing.
 */
int minhttp_proxy(struct config *cfg);

#endif /* __MINHTTP_H */
