#include "./module.h"
#include "./lexer.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>

module modules[255] = {0};
module * module_cache = NULL;

long level[2] = {0};
int recording = 0;
statement_t c_stmt;

static void enumerate_exports(module * m, FILE* out){
    /*fprintf(stderr, "EXPORTS for '%s'\n", m->abs_path);*/
    /*fprintf(stderr, "Package : %s\n", m->name);*/
    export_t * e;

    fprintf(out, "\n/* included from %s */\n", m->abs_path);
    
    for (e = &m->exports[0]; e != NULL; e = e->hh.next){
        /*fprintf(stderr, "\t%s_%s: \"%s\"\n", m->name, e->name, e->declaration);*/
        fprintf(out, "%s\n", e->declaration);
    }
    fprintf(out, "\n");

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
     /*printf("IMPORT: module %s will be aliased as '%s'\n", import->file, import->alias);*/
    import->module = module_parse(import->file);
    enumerate_exports(import->module, m->out);
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
                char * out = malloc(strlen(m->name) + strlen(line) + 1);
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
    return NULL;
}

module * module_parse(const char * filename){
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

    // get full path for caching purposes
    modules[fd].abs_path = realpath(filename, NULL);

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

    
    char * original_name = modules[fd].abs_path;
    char * generated_path = strdup(original_name);
    ext = index(generated_path, '.');
    if (ext){
        *ext = '\0';
    }
    // get generated c file name;
    FILE * out;
    if (strlen(original_name) - strlen(generated_path) >= 2){
        strcat(generated_path, ".c");
        if (strcmp(generated_path, original_name) != 0){
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
    free(generated_path);
    yyset_in(fdopen(fd, "rb"), scanner);
    yyset_out(out, scanner);
    modules[fd].out = out;
    yylex(scanner);
    yylex_destroy(scanner);

    chdir(cwd); // return to where we were before

    module * m = malloc(sizeof(module));
    memcpy(m, &modules[fd], sizeof(module));

    free(fname);
    free(original_name);
    return m;
}

void module_unalias(FILE*current, const char * line){
    module * m = &modules[fileno(current)];
    import_t * imports = m->imports;
    char * sep = strchr(line, '.');
    char * alias = "";
    if (sep){
        size_t alias_len = sep - line + 1;
        alias = malloc(alias_len);
        strncpy(alias, line, alias_len - 1);
        alias[alias_len - 1] = '\0';
        import_t * import = NULL;
        if (imports != NULL){
            HASH_FIND_STR(imports, alias, import);
        }
        if (import && import->module && import->module->name){
            sep++;
            //TODO: check to ensure export exists
            fprintf(m->out, "%s_%s", import->module->name, sep);
            return;
        }
    }
    fprintf(m->out,"%s",line);
}

void module_name(FILE*current, const char * line){
    module * m = &modules[fileno(current)];
    import_t * imports = m->imports;

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
    module * m = &modules[fileno(current)];

    if (add > 0 && type == 0 && c_stmt.type == EXPORT){
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
    module * m = &modules[fileno(current)];
    import_t * imports = m->imports;
    memset(&c_stmt, 0, sizeof(c_stmt));
    c_stmt.type = EXPORT;
    c_stmt.export.num_tokens = 0;

    level[1] = 0;
    recording = 1;
}

void module_export_try_end(FILE*current, const char * line){
    module * m = &modules[fileno(current)];
    import_t * imports = m->imports;


    if (recording && level[1] == 0 && c_stmt.type == EXPORT){
        recording = 0;
        if (c_stmt.export.name == NULL){
            c_stmt.export.name = c_stmt.export.tokens[c_stmt.export.num_tokens - 1];
        }
        recording = 0;
        export_t * e = malloc(sizeof(export_t));

        e->name = c_stmt.export.name;

        e->declaration = malloc(c_stmt.export.tokens_length + strlen(m->name) + 3);
        e->declaration[0] = '\0';
        int i;
        for (i = 0; i < c_stmt.export.num_tokens; i++){
            if ( c_stmt.export.tokens[i] == c_stmt.export.name){
                strcat(e->declaration, m->name);
                strcat(e->declaration, "_");
            }
            strcat(e->declaration, c_stmt.export.tokens[i]);
            if ( c_stmt.export.tokens[i] != c_stmt.export.name){
                free(c_stmt.export.tokens[i]);
            }
        }
        fprintf(m->out,"%s%s",e->declaration, line);
        strcat(e->declaration, ";");
        HASH_ADD_STR(m->exports, name, e);
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
    } else {
        module_prefix(current, line);
    }
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
    }
}
