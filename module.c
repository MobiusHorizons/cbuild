#include "./module.h"
#include "./lexer.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>


#define SEP '/'

int count_levels(const char * path){
  int i = 0, levels = 0;
  bool escaped = false;

  while(path[i] != '\0'){
    if (escaped) {
      escaped = false;
      continue;
    }
    if (path[i] == '\\') escaped = true;
    if (path[i] == SEP) levels++;
    i++;
  }

  return levels;
}

char * relative(const char * from, const char * to){
  // determine shared prefix;

  int i = 0, last_sep = 0;
  while(from[i] != '\0' && to[i] != '\0' && from[i] == to[i]){
    if (from[i] == SEP){
      last_sep = i;
    }
    i++;
  }
  int dots = count_levels(&from[last_sep +1]);

  char * rel = malloc((dots == 0 ? 2 : (dots * 3)) + strlen(&to[last_sep]) + 1);
  rel[0] = 0;
  if (dots == 0){
      strcpy(rel, "./");
  }
  while (dots--){
    strcat(rel, "../");
  }
  strcat(rel, &to[last_sep + 1]);
  return rel;
}

static void enumerate_exports(module * m, FILE* out);

module modules[255] = {0};
module * module_cache = NULL;

long level[2] = {0};
int recording = 0;
statement_t c_stmt;

const char* type_names[] = {
    "VARIABLE",
    "TYPE",
    "UNION",
    "STRUCT",
    "ENUM"
};

static void write_headers(module * dep, module * m){
    if (dep->header_path == NULL){
        char * header_path = relative(m->abs_path, dep->abs_path); 
        char * ext = index(basename(header_path), '.');
        if (ext){
            *ext = '\0';
        }
        strcat(header_path, ".h");

        FILE* header = fopen(header_path, "w");
        dep->header_path = strdup(header_path);
        free(header_path);
        
        enumerate_exports(dep, header);

        fclose(header);
    }
    fprintf(m->out, "#include \"%s\"\n", dep->header_path); 
}

#define GUARDS_HEAD "#ifndef _%s_\n#define _%s_\n\n"
#define GUARDS_FOOT "#endif\n"
static void enumerate_exports(module * m, FILE* out){
    /*fprintf(stderr, "EXPORTS for '%s'\n", m->abs_path);*/
    /*fprintf(stderr, "Package : %s\n", m->name);*/
    export_t * e;

    fprintf(out, GUARDS_HEAD, m->name, m->name);
    if (m->verbose){
        fprintf(stderr, GUARDS_HEAD, m->name, m->name);
    }

    for (e = &m->exports[0]; e != NULL; e = e->hh.next){
        if (m->verbose){
            fprintf(stderr, "%s_%s (%s): \"%s\"\n\n", m->name, e->name, type_names[e->type], e->declaration);
        }
        fprintf(out, "%s\n\n", e->declaration);
    }
    fprintf(out, "\n");
    fprintf(out, GUARDS_FOOT);
    if (m->verbose){
        fprintf(stderr, GUARDS_FOOT);
    }
}

static import_t * parse_import(import_t * imports, const char * line){
    const char * id_start = line + strlen("import");
    // trim whitespace
    while(isspace(*id_start)) id_start++;

    unsigned id_length = 0;
    while(!isspace(id_start[id_length])) id_length++;

    char * alias = (char*)malloc(id_length + 1);
    strncpy(alias, id_start, id_length);
    alias[id_length] = '\0';

    import_t * import = NULL;
    if (imports != NULL){
        HASH_FIND_STR(imports, alias, import);
    }

    if (import){
        fprintf(stderr,"'%s' has already been imported from %s\n", alias, import->file);
        exit(1);
    }

    import = malloc(sizeof(import_t));
    import->alias = alias;

    const char * module_start  = strstr(line, "from") + strlen("from");
    const char * module_end    = line + strlen(line) - 1;

    while(module_start < module_end && *module_start != '"') module_start++;
    module_start++;
    unsigned module_length = 0;

    while(module_end > module_start && (*module_end != '"')) module_end--;
    module_length = module_end - module_start;

    import->file = (char *) malloc(module_length + 1);
    strncpy(import->file, module_start, module_length);
    import->file[module_length] = '\0';

    return import;
}

void module_imports(FILE*current, const char * line){
    module * m = &modules[fileno(current)];
    import_t * imports = m->imports;
    import_t * import = parse_import(imports, line);
#ifdef DEBUG
     /*printf("IMPORT: module %s will be aliased as '%s'\n", import->file, import->alias);*/
#endif
    import->module = module_parse(import->file, m->verbose);
    write_headers(import->module, m);
    /*enumerate_exports(import->module, m->out);*/
    HASH_ADD_STR(m->imports, alias, import);
}

char * module_prefix(FILE* current, const char * line){
    /*if (recording) return;*/
    module * m = &modules[fileno(current)];

    if (m->exports){
        export_t * e = NULL;
        HASH_FIND_STR(m->exports, line, e);
        if (e && m->name){
            if (recording){
                char * out = malloc(strlen(m->name) + strlen(line) + 2);
                strcpy(out, m->name);
                strcat(out, "_");
                strcat(out, line);
                return out;
            } else {
                fprintf(m->out,"%s_%s", m->name, line);
                return NULL;
            }
        }
    }
    
    if (recording){
        return strdup(line);
    }
    fprintf(m->out,"%s", line);
#ifdef DEBUG
    printf("%s", line);
#endif
    return NULL;
}

module * module_parse(const char * filename, bool verbose){
    char * abs_path = realpath(filename, NULL);

    module * m = NULL;
    HASH_FIND_STR(module_cache, abs_path, m);
    if (m != NULL) return m;

    yyscan_t scanner;
    yylex_init(&scanner);

    char buffer[2][PATH_MAX];

    char * cwd = getcwd(buffer[0], PATH_MAX);

    int fd = open(filename, O_RDONLY);
    if (fd < 0){
        fprintf(stderr, "Error parsing %s\n", filename);
        exit(-1);
    }
    
    /*printf("parsing %s, fd = %d\n", filename, fd);*/
    char * fname = strdup(filename);
    modules[fd].imports = NULL;
    modules[fd].verbose = verbose;

    // get full path for caching purposes
    modules[fd].abs_path = abs_path;

    // set the current directory to the parent directory of the open file
    strcpy(buffer[1], filename);
    char * new_cwd = dirname(buffer[1]);
    chdir(new_cwd);

    char * package = basename(fname);
    char * ext = index(package, '.');
    if (ext){
        *ext = '\0';
    }
    modules[fd].name = strdup(package);
    modules[fd].is_exported = false;
    
    char * generated_path = strdup(abs_path);
    ext = index(generated_path, '.');
    if (ext){
        *ext = '\0';
    }
    // get generated c file name;
    FILE * out = NULL;
    if (strlen(abs_path) - strlen(generated_path) >= 2){
        strcat(generated_path, ".c");
        if (strcmp(generated_path, abs_path) != 0){
            out = fopen(generated_path, "w");
        }
    }
    if (out == NULL){
        generated_path = realloc(generated_path, strlen(generated_path) + strlen(".generated"));
        char * ext = rindex(generated_path, '.');
        *ext = '\0';
        strcat(generated_path, ".generated.c");
        out = fopen(generated_path, "w");
    }

    modules[fd].generated_path = strdup(generated_path);
    modules[fd].header_path = NULL;
    free(generated_path);

    FILE* in = fdopen(fd, "rb");
    yyset_in(in, scanner);
    yyset_out(out, scanner);
    modules[fd].out = out;
    yylex(scanner);
    yylex_destroy(scanner);

    chdir(cwd); // return to where we were before

    m = malloc(sizeof(module));
    memcpy(m, &modules[fd], sizeof(module));

    HASH_ADD_STR(module_cache, abs_path, m);
    fclose(in);
    fclose(out);
    free(fname);
    /*free(original_name);*/
    return m;
}

void module_unalias(FILE*current, const char * line){
    module * m = &modules[fileno(current)];
    import_t * imports = m->imports;
    char * sep = strchr(line, '.');
    char * alias = NULL;
    if (sep){
        size_t alias_len = sep - line + 1;
        alias = malloc(alias_len);
        strncpy(alias, line, alias_len - 1);
        alias[alias_len - 1] = '\0';
        import_t * import = NULL;
        if (imports != NULL){
            HASH_FIND_STR(imports, alias, import);
        }
        free(alias);
        if (import && import->module && import->module->name){
            sep++;
            //TODO: check to ensure export exists
            char * unaliased = malloc(strlen(import->module->name) + strlen(sep) + 2);
            sprintf(unaliased, "%s_%s", import->module->name, sep);
            if (recording && c_stmt.type == EXPORT){
               printf("module %s exports %s\n", m->name, unaliased); 
            }
            module_export_declaration(current, unaliased);
            free(unaliased);
            return;
        }
    }
    fprintf(m->out,"%s",line);
#ifdef DEBUG
    printf("%s",line);
#endif
}

void module_name(FILE*current, const char * line){
    module * m = &modules[fileno(current)];

    const char * end = line + strlen(line);
    while(end > line && *end != '"') end--;

    const char * start = end - 1;
    while(start > line && *start != '"') start--;
    start++;

    size_t length = end - start;
    // free previous name
    free(m->name);
    m->name = malloc(length + 1);
    strncpy(m->name, start, length);
    m->name[length] = '\0';
}

void module_count(FILE* current, int add, int type, const char * line){
    /*module * m = &modules[fileno(current)];*/

    if (add > 0 && type == 0 && c_stmt.type == EXPORT && level[1] == 0){
        c_stmt.export.is_function = 1;
        if (c_stmt.export.name == NULL){
            c_stmt.export.name = c_stmt.export.tokens[c_stmt.export.num_tokens - 1];
        }

    }
    if (add > 0 && type == 1 && c_stmt.type == EXPORT && c_stmt.export.is_function){
        return module_export_try_end(current, line);
    }
    level[type] += add;
    module_export_declaration(current, line);
}

void module_export_start(FILE*current, const char * line){
    /* free accumulated tokens */
    int i;
    for (i = 0; i < c_stmt.export.num_tokens; i++){
        free(c_stmt.export.tokens[i]);
    }
    free(c_stmt.export.tokens);
    c_stmt.export.num_tokens = 0;
    c_stmt.export.tokens = NULL;

    memset(&c_stmt, 0, sizeof(c_stmt));
    c_stmt.type = EXPORT;
    c_stmt.export.num_tokens = 0;

    level[1] = 0;
    recording = 1;
}

void module_export_try_end(FILE*current, const char * line){
    module * m = &modules[fileno(current)];


    if (recording && level[1] == 0 && c_stmt.type == EXPORT){
        recording = 0;
        if (c_stmt.export.name == NULL){
            c_stmt.export.name = c_stmt.export.tokens[c_stmt.export.num_tokens - 1];
        }
        recording = 0;
        export_t * e = malloc(sizeof(export_t));

        e->name = strdup(c_stmt.export.name);
        e->type = c_stmt.export.type;

        e->declaration = malloc(c_stmt.export.tokens_length + strlen(m->name) + 3);
        e->declaration[0] = '\0';

        int i;
        for (i = 0; i < c_stmt.export.num_tokens; i++){
            if ( c_stmt.export.tokens[i] == c_stmt.export.name){
                strcat(e->declaration, m->name);
                strcat(e->declaration, "_");
            }
            strcat(e->declaration, c_stmt.export.tokens[i]);
            free(c_stmt.export.tokens[i]);
        }
        fprintf(m->out,"%s%s",e->declaration, line);
#ifdef DEBUG
        printf("%s%s",e->declaration, line);
#endif
        strcat(e->declaration, ";");
        HASH_ADD_STR(m->exports, name, e);
        free(c_stmt.export.tokens);
        c_stmt.export.num_tokens = 0;
        c_stmt.export.tokens = NULL;
    } else {
        module_export_declaration(current, line);
    }


}

void module_ID(FILE*current, const char * line){
    if (recording && c_stmt.type == EXPORT){
        /*fprintf(stderr, "\nID: %s, (%lu, %lu)\n", line, c_stmt.export.num_tokens, c_stmt.export.tokens_length);*/
        char * token = module_prefix(current, line);
        c_stmt.export.num_tokens++;
        c_stmt.export.tokens = realloc(c_stmt.export.tokens, c_stmt.export.num_tokens * sizeof(char*));
        c_stmt.export.tokens[c_stmt.export.num_tokens - 1] = strdup(token);
        c_stmt.export.tokens_length += strlen(token);
        /*fprintf(stderr, "ID: %s, (%lu, %lu)\n", c_stmt.export.tokens[c_stmt.export.num_tokens-1], c_stmt.export.num_tokens, c_stmt.export.tokens_length);*/
        free(token);
    } else {
        free(module_prefix(current, line));
    }
}

void module_export_type(FILE* current, const char * line, enum export_type type){
    if (recording){
        if (c_stmt.type == EXPORT){
            if (c_stmt.export.type == 0){
                c_stmt.export.type = type;
            }
        }
    }
    module_export_declaration(current, line);
}

void module_export_declaration(FILE* current, const char * line){
    if (recording){
        if (c_stmt.type == EXPORT){
            c_stmt.export.num_tokens++;
            c_stmt.export.tokens = realloc(c_stmt.export.tokens, c_stmt.export.num_tokens * sizeof(char*));
            c_stmt.export.tokens[c_stmt.export.num_tokens - 1] = strdup(line);
            c_stmt.export.tokens_length += strlen(line);
        }
    } else {
        module * m = &modules[fileno(current)];
        fprintf(m->out, "%s", line);
#ifdef DEBUG
        printf("%s", line);
#endif
    }
}
