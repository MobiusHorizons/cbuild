#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include "parser/colors.h"


#include "deps/hash/hash.h"

#include "package/package.h"
#include "package/export.h"
#include "package/import.h"
#include "package/atomic-stream.h"
#include "utils/utils.h"
#include "deps/stream/stream.h"

static const char * ops[] = {
  ":=",
  "?=",
  "+=",
};

int makefile_make(package_t * pkg, char * makefile) {
  if (pkg == NULL) return -1;
  char * target = basename(strdup(pkg->generated));
  char * cmd;
  asprintf(&cmd, "make -f %s %.*s", makefile, (int)(strlen(target) - 2), target);

  free(makefile);
  return system(cmd);
}

int makefile_clean(package_t * pkg, char * makefile) {
  if (pkg == NULL) return -1;
  char * target = basename(strdup(pkg->generated));
  char * cmd;
  asprintf(&cmd, "make -f %s CLEAN_%.*s", makefile, (int)(strlen(target) - 2), target);

  int result = system(cmd);

  printf("unlink: %s\n", makefile);
  unlink(makefile);
  free(makefile);
  return result;
}

char * write_deps(package_t * pkg, package_t * root, stream_t * out, char * deps) {
  if (pkg == NULL || pkg->exported) return deps;
  pkg->exported = true;

  char * source = utils_relative(root->generated, pkg->generated);
  char * object = strdup(source);
  object[strlen(source) - 1] = 'o';

  size_t len = deps == NULL ? 0 : strlen(deps);
  deps = realloc(deps, len + strlen(object) + 2);
  sprintf(deps + len, " %s", object);

  stream_printf(out, "#dependencies for package '%s'\n", utils_relative(root->source_abs, pkg->generated));

  int i;
  for (i = 0; i < pkg->n_variables; i++) {
    package_var_t v = pkg->variables[i];
    stream_printf(out, "%s %s %s\n", v.name, ops[v.operation], v.value);
  }

  stream_printf(out, "%s: %s", object, source);

  if (pkg->deps == NULL) {
  stream_printf(out,"\n\n");
    return deps;
  }

  hash_each(pkg->deps, {
      package_import_t * dep = (package_import_t *) val;
      if (dep && dep->pkg && dep->pkg->header_abs)
        stream_printf(out, " %s", utils_relative(root->source_abs, dep->pkg->header_abs));
  });
  stream_printf(out,"\n\n");

  hash_each(pkg->deps, {
      package_import_t * dep = (package_import_t *)val;
      deps = write_deps(dep->pkg, root, out, deps);
  });

  return deps;
}

char * get_makefile_name(const char * path) {
  char * name = strdup(path);
  name[strlen(name) - strlen("module.c")] = 0;
  strcat(name, "mk");
  return name;
}

char * makefile_write(package_t * pkg, const char * name) {
  char * target = NULL;
  char * mkfile_name = get_makefile_name(name);
  stream_t * mkfile = atomic_stream_open(mkfile_name);
  char * deps = write_deps(pkg, pkg, mkfile, NULL);

  if (strcmp(pkg->name, "main") == 0) {
    char * buf = strdup(pkg->generated);
    char * base = basename(buf);
    asprintf(&target, "%.*s", (int)strlen(base) - 2, base);
    free(buf);

    stream_printf(mkfile, "%s:%s\n",target, deps);
    stream_printf(mkfile, "\t$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS)%s -o %s\n\n", deps, target);
  } else {
    target = malloc(strlen(pkg->name) + 3);
    strcpy(target, pkg->name);
    strcat(target, ".a");

    stream_printf(mkfile, "%s:%s\n",target, deps);
    stream_printf(mkfile, "\tar rcs $@ $^\n\n", deps, target);
  }

  stream_printf(mkfile, "CLEAN_%s:\n", target);
  stream_printf(mkfile, "\trm -rf %s%s\n", target, deps);

  stream_close(mkfile);
  free(deps);

  return mkfile_name;
}
