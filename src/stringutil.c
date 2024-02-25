#include "stringutil.h"

#include <string.h>

int sstartswith(const char *s, const char *key) {
	return strncmp(s, key, strlen(key)) == 0;
}
