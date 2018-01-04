#define _BSD_SOURCE
#define _GNU_SOURCE
#define VERSION "v2.0.0"



#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>



#include "package/index.h"
#include "package/package.h"
#include "package/import.h"
#include "makefile.h"
#include "cli.h"

package_t * generate(const char * filename, bool force, bool no_output) {
  char * error = NULL;
  package_t * pkg = index_new(filename, &error, force, no_output);

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

  package_t * root = generate(cli->argv[0], opts->force, false);
  if (root == NULL) exit(-1);
  return 0;
}

int do_build(cli_t * cli, char * cmd, void * arg) {
  options_t * opts = (options_t*) arg;
  if (cli->argc < 1) {
    fprintf(stderr, "no root module specified\n");
    return -1;
  }

  package_t * root = generate(cli->argv[0], opts->force, false);
  if (root == NULL) exit(-1);

  char * mkfile_name = makefile_write(root, cli->argv[0]);
  int result = makefile_make(root, mkfile_name);
  if (result != 0) exit(result);
  return 0;
}

void clean_generated(package_t * pkg) {
  if (pkg == NULL || pkg->c_file || pkg->exported == false) return;
  pkg->exported = false;

  if (pkg->generated) {
    printf("unlink: %s\n", pkg->generated);
    unlink(pkg->generated);
  }
  if (pkg->header) {
    printf("unlink: %s\n", pkg->header);
    unlink(pkg->header);
  }

  hash_each_val(pkg->deps, {
    package_import_t * imp = (package_import_t *) val;
    clean_generated(imp->pkg);
  });
}

int do_clean(cli_t * cli, char * cmd, void * arg) {
  options_t * opts = (options_t*) arg;

  if (cli->argc < 1) {
    fprintf(stderr, "no root module specified\n");
    return -1;
  }

  package_t * root = generate(cli->argv[0], opts->force, true);
  if (root == NULL) exit(-1);

  char * mkfile_name = makefile_write(root, cli->argv[0]);
  int result = makefile_clean(root, mkfile_name);
  clean_generated(root);
  if (result != 0) exit(result);
  return 0;
}

int main(int argc, const char ** argv){
  options_t options = {0};

  cli_t * c = cli_new("<root module>");
  cli_flag_bool(c, &options.force, (cli_flag_options) {
      .long_name   = "force",
      .short_name  = "f",
      .description = "force rebuilding assets",
  });

  cli_command(c, "build",    do_build,    "generate code and build",      true,  &options);
  cli_command(c, "generate", do_generate, "generate .c .h and .mk files", false, &options);
  cli_command(c, "clean",    do_clean,    "clean generated files",        false, &options);

  return cli_parse(c, argc, argv);
}