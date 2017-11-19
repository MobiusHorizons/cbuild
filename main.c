#include "./module.h"
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "./relative.h"

char cwd_buffer[PATH_MAX];
/*char path_buffer[PATH_MAX];*/
char * cwd;

struct config {
    bool verbose;
    bool run_make;
    bool free;
    bool parse_only;
};

enum command {
    cmd_build = 0,
    cmd_generate,
    cmd_clean,
};

struct generate {
    module * root;
    char   * makefile_path;
    char   * libname;
};

static char * variable_operators[] = {
    ":=",
    "?=",
    "+=",
    "=",
};

static void print_dependencies(module * m, FILE* out, module * root){
    import_t * i;

    char * rel_path = relative(root->abs_path, m->generated_path); 
    fprintf(out, "# dependencies for %s\n", rel_path);


    char * object_path = strdup(rel_path);
    char * ext = index(basename(object_path), '.');
    if (ext){
        object_path[strlen(object_path) - strlen(ext)] = '\0';
    }

    struct variable *v;
    for (v = &m->variables[0]; v != NULL; v = v->hh.next){
        fprintf(out, "%s %s %s\n", v->name, variable_operators[v->type], v->value);
    }

    fprintf(out, "%s.o: %s", object_path, rel_path);

    free(rel_path);
    for (i = &m->imports[0]; i != NULL; i = i->hh.next){
        switch(i->type){
            case module_import:
                if (i->module && i->module->header_path){
                    rel_path = relative(root->abs_path, i->module->header_path);
                    fprintf(out, " %s", rel_path);
                    free(rel_path);
                }
                break;
            case c_file:
                rel_path = relative(root->abs_path, i->file);
                fprintf(out, " %s", rel_path);
                free(rel_path);
                break;
            default:
            break;
        }
    }
    fprintf(out, "\n\n");

    free(object_path);
}

static char * c_dependency(import_t * i, FILE* out, char * objects, module * root){
    if (i->type != c_file) return objects;
    if (i->module && i->module->is_exported) return objects;

    char * rel_path = relative(root->abs_path, i->file); 
    fprintf(out, "# dependencies for %s\n", rel_path);


    char * object_path = strdup(rel_path);
    char * ext = index(basename(object_path), '.');
    if (ext){
        object_path[strlen(object_path) - strlen(ext)] = '\0';
    }

    fprintf(out, "%s.o: %s \n\n", object_path, rel_path);
    objects = realloc(objects, strlen(objects) + strlen(object_path) + strlen(" .o") + 1);

    strcat(objects, " ");
    strcat(objects, object_path);
    strcat(objects, ".o");
    free(object_path);
    i->module->is_exported = true;
    return objects;
}

static char * walk_dependencies(module * m, FILE* out, char * objects, module * root){
    if (m == NULL || m->is_exported) return objects;

    print_dependencies(m, out, root);
    import_t * i;
    for (i = &m->imports[0]; i != NULL; i = i->hh.next){
        if (i->type == module_import){
            objects = walk_dependencies(i->module, out, objects, root);
        } else if (i->type == c_file){
            objects = c_dependency(i, out, objects, root);
        }
    }

    char * object_path = relative(root->abs_path, m->generated_path); 
    char * ext = index(basename(object_path), '.');
    if (ext){
        object_path[strlen(object_path) - strlen(ext)] = '\0';
    }
    

    objects = realloc(objects, strlen(objects) + strlen(object_path) + strlen(" .o") + 1);

    strcat(objects, " ");
    strcat(objects, object_path);
    strcat(objects, ".o");
    free(object_path);

    m->is_exported = true;
    return objects;
}

void write_headers(module * root, const char * path){
    char * header_path = strdup(path);
    char * ext = index(header_path, '.');
    if (ext){
        *ext = '\0';
    }
    strcat(header_path, ".h");

    FILE* header = fopen(header_path, "w");
    free(header_path);

    export_t * e;
    for (e = &root->exports[0]; e != NULL; e = e->hh.next){
        fprintf(header,"%s\n", e->declaration);
    }

    fclose(header);
}

struct generate generate(struct config conf, const char * root_file) {
  module * root = module_parse(root_file, conf.verbose, conf.parse_only);
  strcpy(cwd_buffer, root->abs_path);
  cwd = strdup(dirname(cwd_buffer));

  char * libname = strdup(root->abs_path + strlen(cwd) + 1);
  char * ext = index(basename(libname), '.');
  if (ext){
      libname[strlen(libname) - strlen(ext)] = '\0';
  }

  char * makefile_path = malloc(strlen(root->abs_path) + 3);
  strcpy(makefile_path, root->abs_path);

  ext = index(makefile_path, '.');
  if (ext){
      *ext = '\0';
  }

  strcat(makefile_path, ".mk");
  FILE* makefile = fopen(makefile_path, "w");

  char * objects = walk_dependencies(root, makefile, strdup(""), root);

  if (strcmp(root->name, "main") == 0){
      fprintf(makefile, "%s:%s\n", libname, objects);
      fprintf(makefile, "\t$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) %s -o %s\n", objects, libname);
  } else {
      strcat(libname, ".a");
      fprintf(makefile, "%s:%s\n", libname, objects);
      fprintf(makefile, "\tar rcs $@ $^\n");

      if (root->exports){
          write_headers(root, makefile_path);
      }

  }
  fprintf(makefile, "\nCLEAN_%s:\n", libname);
  fprintf(makefile, "\trm -rf %s %s\n", libname, objects);
  fclose(makefile);
  free(objects);

  if (conf.free){
      free(libname);
      free(makefile_path);

      struct generate paths = {0};
      return paths;
  }

  struct generate paths = {
      .root          = root,
      .libname       = libname,
      .makefile_path = makefile_path,
  };

  return paths;
}

int build(struct config conf, const char * root_file) {
    struct generate paths = generate(conf, root_file);

    pid_t p = fork();
    if (p){
        wait(NULL);
    } else {
        char * makefile_dir = dirname(strdup(paths.makefile_path));
        chdir(makefile_dir);
        free(makefile_dir);

        execlp("make", "make", "-f", basename(paths.makefile_path), paths.libname, NULL);
    }


    free(paths.libname);
    free(paths.makefile_path);
    return 0;
}

static void clean_recursive(module * m, module * root, bool verbose) {
    if (m == NULL) return;

    import_t * i;
    for (i = &m->imports[0]; i != NULL; i = i->hh.next) {
        if (i->type == module_import){
            clean_recursive(i->module, root, verbose);
        }
    }

    if (m->generated_path) {
        if (m->header_path){
            if (verbose) fprintf(stderr, "unlink %s\n", m->header_path);
            unlink(m->header_path);
        }

        unlink(m->generated_path);
        if (verbose) fprintf(stderr, "unlink %s\n", m->generated_path);

        m->generated_path = NULL;
    }
}

const char * clean(struct config conf, const char * root_file) {
    conf.parse_only = true;
    struct generate paths = generate(conf, root_file);

    pid_t p = fork();
    if (p){
        wait(NULL);
    } else {
        char * makefile_dir = dirname(strdup(paths.makefile_path));
        chdir(makefile_dir);
        free(makefile_dir);

        char * clean_cmd = malloc(strlen(paths.libname + strlen("CLEAN_") + 1));
        strcpy(clean_cmd, "CLEAN_");
        strcat(clean_cmd, paths.libname);

        if (conf.verbose){
            execlp("make", "make", "-f", basename(paths.makefile_path), clean_cmd, NULL);
        } else {
            execlp("make", "make", "-s", "-f", basename(paths.makefile_path), clean_cmd, NULL);
        }
    }

    clean_recursive(paths.root, paths.root, conf.verbose);
    unlink(paths.makefile_path);

    free(paths.libname);
    free(paths.makefile_path);

    return 0;
}

void usage(const char * name){
    fprintf(stderr, "\n");
    fprintf(stderr, "    %s [command] [options] <module>\n", name);
    fprintf(stderr, "\n    Options:\n\n");
    {
        fprintf(stderr, "        -v         verbose\n");
    }
    fprintf(stderr, "\n    Commands:\n\n");
    {
        fprintf(stderr, "        build      generates source files and builds the module (default)\n");
        fprintf(stderr, "        generate   generates source files\n");
        fprintf(stderr, "        clean      clean all generated sources, object files, and executables");
    }
    fprintf(stderr, "\n");
}

int main(int argc, char **argv){
  const char * executable = argv[0];
  ++argv; --argc;	/* skip over program name */
  char * root_file = NULL;
  struct config conf = {0};

  enum command cmd = cmd_build;

  /* parse CLI */
  while( argc > 0){
      if ( argv[0][0] == '-' && strlen(*argv) == 2){
          switch(argv[0][1]) {
              case 'g':
                  conf.run_make = true;
                  break;
              case 'v':
                  conf.verbose = true;
                  break;
              default:
                  fprintf(stderr, "Unknown option (%s)\n", *argv);
          }
      } else {
          if (strcmp(*argv, "build") == 0){
              cmd = cmd_build;
          } else if (strcmp(*argv, "generate") == 0){
              cmd = cmd_generate;
          } else if (strcmp(*argv, "clean") == 0){
              cmd = cmd_clean;
          } else {
              root_file = *argv;
          }
      }
      argv++; argc--;
  }

  if ( root_file == NULL ){
      usage(executable);
      return -1;
  }

  switch(cmd) {
      case cmd_clean:
          clean(conf, root_file);
          break;
      case cmd_generate:
          conf.free = true;
          generate(conf, root_file);
          break;
      case cmd_build:
      default:
          build(conf, root_file);
  }

  free(cwd);
}

