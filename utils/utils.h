#ifndef _package_utils_
#define _package_utils_

#include <stdbool.h>
char * utils_relative(const char * from, const char * to);
bool utils_newer(const char * a, const char * b);

#endif
