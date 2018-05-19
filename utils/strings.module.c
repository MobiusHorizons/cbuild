#include <string.h>
export {
#include <stdlib.h>
}

export char * dup(const char * value) {
	if (value == NULL) {
		return NULL;
	} else {
		return strdup(value);
	}
}

export size_t len(const char * value) {
	if (value == NULL) {
		return 0;
	} else {
		return strlen(value);
	}
}

