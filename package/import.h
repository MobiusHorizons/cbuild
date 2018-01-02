#ifndef _package_package_import_
#define _package_package_import_

#include <stdbool.h>

#include "package.h"

typedef struct {
  char      * alias;
  char      * filename;
  bool        c_file;
  package_t * pkg;
} package_import_t;

package_import_t * package_import_add(char * alias, char * filename, package_t * parent, char ** error);
package_import_t * package_import_add_c_file(package_t * parent, char * filename, char ** error);

#endif
