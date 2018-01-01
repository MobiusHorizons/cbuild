package "main";

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <libgen.h>
#include "parser/colors.h"

import Pkg     from "package/index.module.c";
import Package from "package/package.module.c";

void make(Package.t * pkg) {
  char * target = basename(strdup(pkg->generated));
  char * makefile;
  char * cmd;
  asprintf(&makefile, "%.*s.mk", (int)(strlen(pkg->generated) - 2), pkg->generated);
  asprintf(&cmd, "make -f %s %.*s", makefile, (int)(strlen(target) - 2), target);

  printf("Running: '%s'\n", cmd);

  global.free(makefile);
  system(cmd);
}

int main(int argc, char ** argv){
  if (argc <= 1) {
    printf("no file specified\n");
    return 1;
  }

  char * error;
  Package.t * pkg = Pkg.new(argv[1], &error);
  make(pkg);
}
