#include "./module.h"
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

char cwd_buffer[PATH_MAX];
char path_buffer[PATH_MAX];
char * cwd;

struct config {
    bool verbose;
    bool run_make;
};

static void print_dependencies(module * m, FILE* out){
    size_t prefix_len = strlen(cwd) + 1;
    fprintf(out, "# dependencies for %s\n", m->abs_path + prefix_len);

    /*fprintf(out, "%s: %s\n", m->generated_path, m->abs_path + prefix_len);*/
    /*fprintf(out, "\t mpp %s\n\n", m->abs_path + prefix_len);*/

    import_t * i;

    strcpy(path_buffer, m->generated_path + prefix_len);
    char * ext = rindex(path_buffer, '.');
    if (ext){
        *ext = '\0';
    }

    struct variable *v;
    for (v = &m->variables[0]; v != NULL; v = v->hh.next){
        fprintf(out, "%s += \"%s\"\n", v->name, v->value);
    }

    fprintf(out, "%s.o: %s\n\n", path_buffer, m->generated_path + prefix_len);

}

static char * walk_dependencies(module * m, FILE* out, char * objects){
    if (m->is_exported) return objects;

    size_t prefix_len = strlen(cwd) + 1;
    print_dependencies(m, out);
    import_t * i;
    for (i = &m->imports[0]; i != NULL; i = i->hh.next){
        objects = walk_dependencies(i->module, out, objects);
    }

    strcpy(path_buffer, m->generated_path + prefix_len);
    char * ext = rindex(path_buffer, '.');
    if (ext){
        *ext = '\0';
    }

    objects = realloc(objects, strlen(objects) + strlen(path_buffer) + strlen(" .o") + 1);

    strcat(objects, " ");
    strcat(objects, path_buffer);
    strcat(objects, ".o");

    m->is_exported = true;
    return objects;
}

void write_headers(module * root, char * path){
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

int main(int argc, char **argv){
  ++argv; --argc;	/* skip over program name */
  char * root_file = NULL;
  struct config conf = {0};

  /* parse CLI */
  while( argc > 0){
      if ( argv[0][0] == '-' && strlen(*argv) == 2){
          switch(argv[0][1]) {
              case 'm':
                  conf.run_make = true;
                  break;
              case 'v':
                  conf.verbose = true;
                  break;
              default:
                  fprintf(stderr, "Unknown option (%s)\n", *argv);
          }
      } else {
          root_file = *argv;
      }
      argv++; argc--;
  }

  if ( root_file != NULL ){
    module * root = module_parse(root_file, conf.verbose);
    strcpy(cwd_buffer, root->abs_path);
    cwd = strdup(dirname(cwd_buffer));

    char * libname = strdup(root->abs_path + strlen(cwd) + 1);
    char * ext = index(libname, '.');
    if (ext){
        *ext = '\0';
    }

    char * makefile_path = malloc(strlen(root->abs_path) + 3);
    strcpy(makefile_path, root->abs_path);

    ext = index(makefile_path, '.');
    if (ext){
        *ext = '\0';
    }

    strcat(makefile_path, ".mk");
    FILE* makefile = fopen(makefile_path, "w");

    char * objects = walk_dependencies(root, makefile, strdup(""));

    if (strcmp(root->name, "main") == 0){
        fprintf(makefile, "%s:%s\n", libname, objects);
        fprintf(makefile, "\t$(CC) %s -o %s\n", objects, libname);
    } else {
        strcat(libname, ".a");
        fprintf(makefile, "%s:%s\n", libname, objects);
        fprintf(makefile, "\tar rcs $@ $^\n");

        if (root->exports){
            write_headers(root, makefile_path);
        }

    }
    fclose(makefile);
    /*sync();*/
    /*sync();*/

    if (conf.run_make){
        pid_t p = fork();
        if (p){
            wait(NULL);
        } else {
            chdir(dirname(makefile_path));
            execlp("make", "make", "-f", basename(makefile_path), libname, NULL);
        }
    }
    free(libname);
    free(makefile_path);
    free(cwd);
    free(objects);
  }
}

