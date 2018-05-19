#include <string.h>

#include <stdlib.h>


char * strings_dup(const char * value) {
	if (value == NULL) {
		return NULL;
	} else {
		return strdup(value);
	}
}

size_t strings_len(const char * value) {
	if (value == NULL) {
		return 0;
	} else {
		return strlen(value);
	}
}
