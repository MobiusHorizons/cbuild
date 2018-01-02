package "main";

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <libgen.h>
#include "parser/colors.h"

build depends "deps/hash/hash.c";
#include "deps/hash/hash.h"

import Pkg        from "package/index.module.c";
import Package    from "package/package.module.c";
import pkg_export from "package/export.module.c";
import pkg_import from "package/import.module.c";
import atomic     from "package/atomic-stream.module.c";
import stream     from "../stream/stream.module.c";
import path       from "../relative_path/relative_path.module.c";

void make(Package.t * pkg, char * makefile) {
  if (pkg == NULL) return;
  char * target = basename(strdup(pkg->generated));
  char * cmd;
  asprintf(&cmd, "make -f %s %.*s", makefile, (int)(strlen(target) - 2), target);

  printf("Running: '%s'\n", cmd);

  global.free(makefile);
  system(cmd);
}

char * write_c_deps(pkg_import.t* imp, Package.t * root, stream.t * out, char * deps) {
  if (root == NULL || imp == NULL) return deps;

  char * source = path.relative(root->generated, imp->alias);
  char * object = strdup(source);
  object[strlen(source) - 1] = 'o';

  size_t len = deps == NULL ? 0 : strlen(deps);
  deps = realloc(deps, len + strlen(object) + 2);
  sprintf(deps + len, " %s", object);

  stream.printf(out, "#dependencies for '%s'\n", path.relative(root->source_abs, imp->alias));
  stream.printf(out, "%s: %s\n\n", object, source);

  return deps;
}

char * write_deps(Package.t * pkg, Package.t * root, stream.t * out, char * deps) {
  if (pkg == NULL || pkg->exported) return deps;
  pkg->exported = true;

  // TODO: write variables
  char * source = path.relative(root->generated, pkg->generated);
  char * object = strdup(source);
  object[strlen(source) - 1] = 'o';

  size_t len = deps == NULL ? 0 : strlen(deps);
  deps = realloc(deps, len + strlen(object) + 2);
  sprintf(deps + len, " %s", object);

  stream.printf(out, "#dependencies for package '%s'\n", path.relative(root->source_abs, pkg->generated));
  stream.printf(out, "%s: %s", object, source);

  if (pkg->deps == NULL) {
  stream.printf(out,"\n\n");
    return deps;
  }

  hash_each(pkg->deps, {
      pkg_import.t * dep = (pkg_import.t *) val;
      if (dep && dep->pkg && dep->pkg->header_abs)
        stream.printf(out, " %s", path.relative(root->source_abs, dep->pkg->header_abs));
  });
  stream.printf(out,"\n\n");

  hash_each(pkg->deps, {
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

char * write_makefile(Package.t * pkg, char * name) {
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

int main(int argc, char ** argv){
  if (argc <= 1) {
    printf("no file specified\n");
    return 1;
  }

  char * error = NULL;
  Package.t * pkg = Pkg.new(argv[1], &error);
  if (error != NULL) {
    fprintf(stderr, "%s\n", error);
    return 1;
  }

  char * mkfile_name = write_makefile(pkg, argv[1]);
  make(pkg, mkfile_name);
}
