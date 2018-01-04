#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include "parser/colors.h"

build depends "deps/hash/hash.c";
#include "deps/hash/hash.h"

import Package    from "package/package.module.c";
import pkg_export from "package/export.module.c";
import pkg_import from "package/import.module.c";
import atomic     from "package/atomic-stream.module.c";
import utils      from "utils/utils.module.c";
import stream     from "deps/stream/stream.module.c";

static const char * ops[] = {
  ":=",
  "?=",
  "+=",
};

typedef struct {
  char * makefile;
  char * makefile_dir;
  char * makefile_base;
  char * target;
  char * cwd;
} makevars;

makevars get_makevars(Package.t * pkg, char * makefile) {
  makevars v = {
    .makefile      = makefile,
    .makefile_base = strdup(basename(makefile)),
    .makefile_dir  = strdup(dirname(makefile)),
    .target        = strdup(basename(pkg->generated)),
    .cwd           = getcwd(NULL, 0),
  };

  char * ext = strrchr(v.target, '.');
  *ext = 0;

  chdir(v.makefile_dir);
  return v;
}

int clear_makevars(makevars v, int result) {
  chdir(v.cwd);

  free(v.cwd);
  free(v.target);
  free(v.makefile_dir);
  free(v.makefile_base);
  free(v.makefile);

  return result;
}

export int make(Package.t * pkg, char * makefile) {
  if (pkg == NULL) return -1;
  makevars v = get_makevars(pkg, makefile);

  char * cmd;
  asprintf(&cmd, "make -f %s %s", v.makefile_base,  v.target);

  return clear_makevars(v, system(cmd));
}

export int clean(Package.t * pkg, char * makefile) {
  if (pkg == NULL) return -1;
  makevars v = get_makevars(pkg, makefile);

  char * cmd;
  asprintf(&cmd, "make -f %s CLEAN_%s", v.makefile_base,  v.target);

  return clear_makevars(v, system(cmd));
}

char * write_deps(Package.t * pkg, Package.t * root, stream.t * out, char * deps) {
  if (pkg == NULL || pkg->exported) return deps;
  pkg->exported = true;

  char * source = utils.relative(root->generated, pkg->generated);
  char * object = strdup(source);
  object[strlen(source) - 1] = 'o';

  size_t len = deps == NULL ? 0 : strlen(deps);
  deps = realloc(deps, len + strlen(object) + 2);
  sprintf(deps + len, " %s", object);

  stream.printf(out, "#dependencies for package '%s'\n", utils.relative(root->source_abs, pkg->generated));

  int i;
  for (i = 0; i < pkg->n_variables; i++) {
    Package.var_t v = pkg->variables[i];
    stream.printf(out, "%s %s %s\n", v.name, ops[v.operation], v.value);
  }

  stream.printf(out, "%s: %s", object, source);

  if (pkg->deps == NULL) {
  stream.printf(out,"\n\n");
    return deps;
  }

  hash_each_val(pkg->deps, {
      pkg_import.t * dep = (pkg_import.t *) val;
      if (dep && dep->pkg && dep->pkg->header)
        stream.printf(out, " %s", utils.relative(root->source_abs, dep->pkg->header));
  });
  stream.printf(out,"\n\n");

  hash_each_val(pkg->deps, {
      pkg_import.t * dep = (pkg_import.t *)val;
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

export char * write(Package.t * pkg, const char * name) {
  char * target = NULL;
  char * mkfile_name = get_makefile_name(name);
  stream.t * mkfile = atomic.open(mkfile_name);
  char * deps = write_deps(pkg, pkg, mkfile, NULL);

  if (strcmp(pkg->name, "main") == 0) {
    char * buf = strdup(pkg->generated);
    char * base = basename(buf);
    asprintf(&target, "%.*s", (int)strlen(base) - 2, base);
    free(buf);

    stream.printf(mkfile, "%s:%s\n",target, deps);
    stream.printf(mkfile, "\t$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS)%s -o %s\n\n", deps, target);
  } else {
    target = malloc(strlen(pkg->name) + 3);
    strcpy(target, pkg->name);
    strcat(target, ".a");

    stream.printf(mkfile, "%s:%s\n",target, deps);
    stream.printf(mkfile, "\tar rcs $@ $^\n\n", deps, target);
  }

  stream.printf(mkfile, "CLEAN_%s:\n", target);
  stream.printf(mkfile, "\trm -rf %s%s\n", target, deps);

  stream.close(mkfile);
  free(deps);

  return mkfile_name;
}

