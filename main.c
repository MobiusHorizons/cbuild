#include "./module.h"
#include <string.h>
#include <libgen.h>
#include <limits.h>

char cwd_buffer[PATH_MAX];
char path_buffer[PATH_MAX];
char * cwd;
char * objects;

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

    fprintf(out, "%s.o: %s\n\n", path_buffer, m->generated_path + prefix_len);

}

static void walk_dependencies(module * m, FILE* out){
    size_t prefix_len = strlen(cwd) + 1;
    print_dependencies(m, out);
    import_t * i;
    for (i = &m->imports[0]; i != NULL; i = i->hh.next){
        walk_dependencies(i->module, out);
    }

    strcpy(path_buffer, m->generated_path + prefix_len);
    char * ext = rindex(path_buffer, '.');
    if (ext){
        *ext = '\0';
    }

    objects = realloc(objects, strlen(path_buffer) + 3);
    strcat(objects, " ");
    strcat(objects, path_buffer);
    strcat(objects, ".o");

}

void write_headers(module * root, char * path){
    char * header_path = path;
    char * ext = index(header_path, '.');
    if (ext){
        *ext = '\0';
    }
    strcat(header_path, ".h");

    FILE* header = fopen(header_path, "w");

    export_t * e;
    for (e = &root->exports[0]; e != NULL; e = e->hh.next){
        fprintf(header,"%s\n", e->declaration);
    }

    fclose(header);
}

int main( argc, argv )
int argc;
char **argv;
{
  ++argv, --argc;	/* skip over program name */
  if ( argc > 0 ){
    module * root = module_parse(argv[0]);
    objects = strdup("");
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
    free(makefile_path);

    walk_dependencies(root, makefile);

    if (strcmp(root->name, "main") == 0){
        fprintf(makefile, "%s:%s\n", libname, objects);
        fprintf(makefile, "\t$(CC) %s -o %s\n", objects, libname);
    } else {
        fprintf(makefile, "%s.a:%s\n", libname, objects);
        fprintf(makefile, "\tar rcs $@ $^\n");

        if (root->exports){
            write_headers(root, makefile_path);
        }

    }
    fclose(makefile);

  }
  free(cwd);
  free(objects);
}

