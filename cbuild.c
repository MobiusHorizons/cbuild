

#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include "parser/colors.h"


#include "deps/hash/hash.h"

#include "package/index.h"
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

void make(package_t * pkg, char * makefile) {
  if (pkg == NULL) return;
  char * target = basename(strdup(pkg->generated));
  char * cmd;
  asprintf(&cmd, "make -f %s %.*s", makefile, (int)(strlen(target) - 2), target);

  printf("Running: '%s'\n", cmd);

  free(makefile);
  system(cmd);
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

char * write_makefile(package_t * pkg, char * name) {
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

int main(int argc, char ** argv){
  if (argc <= 1) {
    printf("no file specified\n");
    return -1;
  }

  char * error = NULL;
  package_t * pkg = index_new(argv[1], &error);
  if (error != NULL) {
    fprintf(stderr, "%s\n", error);
    return 1;
  }
  if (pkg == NULL || pkg->errors > 0) return -1;

  char * mkfile_name = write_makefile(pkg, argv[1]);
  make(pkg, mkfile_name);
}