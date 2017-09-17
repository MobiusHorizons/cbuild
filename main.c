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

    char * rel_path = relative(root->abs_path, i->file); 
    fprintf(out, "# dependencies for %s\n", rel_path);


    char * object_path = strdup(rel_path);
    char * ext = index(basename(object_path), '.');
    if (ext){
        object_path[strlen(object_path) - strlen(ext)] = '\0';
    }

    fprintf(out, "%s.o: %s %s.h\n\n", object_path, rel_path, object_path);
    objects = realloc(objects, strlen(objects) + strlen(object_path) + strlen(" .o") + 1);

    strcat(objects, " ");
    strcat(objects, object_path);
    strcat(objects, ".o");
    free(object_path);
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
        fprintf(makefile, "\t$(CC) $(CFLAGS) $(LDFLAGS) %s -o %s\n", objects, libname);
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

    if (conf.run_make){
        pid_t p = fork();
        if (p){
            wait(NULL);
        } else {
            char * makefile_dir = dirname(strdup(makefile_path));
            chdir(makefile_dir);
            free(makefile_dir);

            execlp("make", "make", "-f", basename(makefile_path), libname, NULL);
        }
    }
    free(libname);
    free(makefile_path);
    free(cwd);
    free(objects);
  }
}

