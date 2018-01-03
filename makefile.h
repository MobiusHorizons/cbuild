#ifndef _package_makefile_
#define _package_makefile_

#include "package/package.h"

int makefile_make(package_t * pkg, char * makefile);
int makefile_clean(package_t * pkg, char * makefile);
char * makefile_write(package_t * pkg, const char * name);

#endif
