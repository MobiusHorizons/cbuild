#define _BSD_SOURCE
#define _GNU_SOURCE
#define VERSION "v2.0.0"

package "main";

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

build append CFLAGS "-std=c99";

import Pkg        from "package/index.module.c";
import Package    from "package/package.module.c";
import pkg_import from "package/import.module.c";
import makefile   from "makefile.module.c";
import cli        from "cli.module.c";

Package.t * generate(const char * filename, bool force, bool no_output) {
  char * error = NULL;
  Package.t * pkg = Pkg.new(filename, &error, force, no_output);

  if (pkg == NULL || pkg->errors > 0) return NULL;

  if (error != NULL) {
    fprintf(stderr, "%s\n", error);
    return NULL;
  }
  return pkg;
}

typedef struct {
  bool force;
} options_t;

int do_generate(cli_t * cli, char * cmd, void * arg) {
  options_t * opts = (options_t*) arg;

  if (cli->argc < 1) {
    fprintf(stderr, "no root module specified\n");
    return -1;
  }
  if (opts->force) printf("FORCED REBUILD\n");

  Package.t * root = generate(cli->argv[0], opts->force, false);
  if (root == NULL) exit(-1);
  return 0;
}

int do_build(cli_t * cli, char * cmd, void * arg) {
  options_t * opts = (options_t*) arg;
  if (cli->argc < 1) {
    fprintf(stderr, "no root module specified\n");
    return -1;
  }

  Package.t * root = generate(cli->argv[0], opts->force, false);
  if (root == NULL) exit(-1);

  char * mkfile_name = makefile.write(root, cli->argv[0]);
  int result = makefile.make(root, mkfile_name);
  if (result != 0) exit(result);
  return 0;
}

void clean_generated(Package.t * pkg) {
  if (pkg == NULL || pkg->c_file || pkg->exported == false) return;
  pkg->exported = false;

  if (pkg->generated) {
    printf("unlink: %s\n", pkg->generated);
    global.unlink(pkg->generated);
  }
  if (pkg->header_abs) {
    printf("unlink: %s\n", pkg->header_abs);
    global.unlink(pkg->header_abs);
  }

  hash_each_val(pkg->deps, {
    pkg_import.t * imp = (pkg_import.t *) val;
    clean_generated(imp->pkg);
  });
}

int do_clean(cli_t * cli, char * cmd, void * arg) {
  options_t * opts = (options_t*) arg;

  if (cli->argc < 1) {
    fprintf(stderr, "no root module specified\n");
    return -1;
  }

  Package.t * root = generate(cli->argv[0], opts->force, true);
  if (root == NULL) exit(-1);

  char * mkfile_name = makefile.write(root, cli->argv[0]);
  int result = makefile.clean(root, mkfile_name);
  clean_generated(root);
  if (result != 0) exit(result);
  return 0;
}

int main(int argc, const char ** argv){
  options_t options = {0};

  cli.t * c = cli.new("<root module>");
  cli.flag_bool(c, &options.force, (cli.flag_options) {
      .long_name   = "force",
      .short_name  = "f",
      .description = "force rebuilding assets",
  });

  cli.command(c, "build",    do_build,    "generate code and build",      true,  &options);
  cli.command(c, "generate", do_generate, "generate .c .h and .mk files", false, &options);
  cli.command(c, "clean",    do_clean,    "clean generated files",        false, &options);

  return cli.parse(c, argc, argv);
}
