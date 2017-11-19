#include "./module.h"
#include "./lexer.h"
#include "./relative.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

module * export_module(module * m, char * alias);
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
    "ENUM",
    "MODULE",
};

static void write_headers(module * dep, module * m){
    if (dep->header_path == NULL){
        char * header_path = strdup(dep->abs_path); //relative(m->abs_path, dep->abs_path); 
        char * ext = index(basename(header_path), '.');
        if (ext){
            header_path[strlen(header_path) - strlen(ext)] = '\0';
        }
        strcat(header_path, ".h");

        FILE* header;
        if (dep->is_write) {
            header = fopen(header_path, "w");
        }
        dep->header_path = strdup(header_path);
        free(header_path);
        
        if (dep->is_write) {
            enumerate_exports(dep, header);
            fclose(header);
        }
    }
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
            fprintf(stderr, "%s_%s (%s): \"%s\"\n", m->name, e->name, type_names[e->type], e->declaration);
        }
        fprintf(out, "%s\n", e->declaration);
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
    import->type = module_import;

    return import;
}

void module_imports(FILE*current, const char * line){
    module * m = &modules[fileno(current)];
    import_t * imports = m->imports;
    import_t * import = parse_import(imports, line);
    import->module = module_parse(import->file, m->verbose, m->parse_only);
    write_headers(import->module, m);
    char * header_path = relative(m->abs_path, import->module->header_path);
    if(m->is_write) fprintf(m->out, "#include \"%s\"\n", header_path); 
    free(header_path);
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
                if(m->is_write) fprintf(m->out,"%s_%s", m->name, line);
                return NULL;
            }
        }
    }

    if (recording){
        return strdup(line);
    }
    if(m->is_write) fprintf(m->out,"%s", line);
    return NULL;
}

static int newer(struct stat a, struct stat b){
#ifdef __MACH__
    if (a.st_mtimespec.tv_sec == b.st_mtimespec.tv_sec) {
        return a.st_mtimespec.tv_nsec > b.st_mtimespec.tv_nsec;
    }
    return a.st_mtimespec.tv_sec > b.st_mtimespec.tv_sec;
#else
    if (a.st_mtim.tv_sec == b.st_mtim.tv_sec) {
        return a.st_mtim.tv_nsec > b.st_mtim.tv_nsec;
    }
    return a.st_mtim.tv_sec > b.st_mtim.tv_sec;
#endif
}

module * module_parse(const char * filename, bool verbose, bool parse_only){
    char * abs_path = realpath(filename, NULL);
    /*printf("%s\n", abs_path);*/

    if (abs_path == NULL){
        int errnum = errno;
        fprintf(stderr, "Error resolving \"%s\": %s\n", filename, strerror( errnum ));
        exit(-1);
    }

    module * m = NULL;
    HASH_FIND_STR(module_cache, abs_path, m);
    if (m != NULL) return m;

    yyscan_t scanner;
    yylex_init(&scanner);

    char buffer[2][PATH_MAX];

    char * cwd = getcwd(buffer[0], PATH_MAX);

    int fd = open(filename, O_RDONLY);
    if (fd < 0){
        int errnum = errno;
        fprintf(stderr, "Error opening \"%s\": %s\n", filename, strerror( errnum ));
        exit(-1);
    }

    struct stat source_stat;
    fstat(fd, &source_stat);

    char * fname = strdup(filename);
    modules[fd].imports = NULL;
    modules[fd].exports = NULL;
    modules[fd].variables = NULL;
    modules[fd].verbose = verbose;
    modules[fd].rel_path = strdup(fname);
    import_t * global = malloc(sizeof(import_t));
    global->type   = global_import;
    global->alias  = "global";
    global->module = NULL;
    global->file   = NULL;

    HASH_ADD_STR(modules[fd].imports, alias, global);

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
            struct stat out_stat;
            if (parse_only || (stat(generated_path, &out_stat) == 0 && !newer(source_stat, out_stat))) {
                out = fopen("/dev/null", "a");
                modules[fd].is_write = false;
            } else {
                out = fopen(generated_path, "w");
                modules[fd].is_write = true;
            }
        }
    }
    if (out == NULL){
        generated_path = realloc(generated_path, strlen(generated_path) + strlen(".generated"));
        char * ext = rindex(generated_path, '.');
        *ext = '\0';
        strcat(generated_path, ".generated.c");
        struct stat out_stat;
        if (parse_only || (stat(generated_path, &out_stat) == 0 && !newer(source_stat, out_stat))) {
            out = fopen("/dev/null", "a");
            modules[fd].is_write = false;
        } else {
            out = fopen(generated_path, "w");
            modules[fd].is_write = true;
        }
    }

    modules[fd].generated_path = strdup(generated_path);
    modules[fd].header_path = NULL;
    modules[fd].parse_only = parse_only;
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

    // TODO: figure out recursive dependencies
    HASH_ADD_STR(module_cache, abs_path, m);
    fclose(in);
    fclose(out);
    free(fname);
    /*free(original_name);*/
    return m;
}

void module_unalias(FILE*current, const char * line){
    if (c_stmt.type == EXPORT && c_stmt.export.type == BLOCK) {
        return module_export_declaration(current, line);
    }
    module * m = &modules[fileno(current)];
    import_t * imports = m->imports;
    /*printf("%s\n", line);*/
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
        sep++;
        if (import && import->module && import->module->name){
            export_t * e = NULL;
            HASH_FIND_STR(import->module->exports, sep, e);

            if (e == NULL){
                fprintf(stderr, "%s: Module %s does not export symbol %s\n",
                        m->rel_path,
                        import->module->name, 
                        sep
                );
                exit(-1);
                return;
            }

            char * unaliased;
            switch (e->type) {
                case MODULE:
                    unaliased = malloc(strlen(e->key) + strlen(sep) + 2);
                    sprintf(unaliased, "%s_%s", e->key, sep);
                    break;
                default:
                    unaliased = malloc(strlen(import->module->name) + strlen(sep) + 2);
                    sprintf(unaliased, "%s_%s", import->module->name, sep);
            }

            if (recording && c_stmt.type == EXPORT){
                // add the header for this symbol to our header file
                export_module(m, import->file);
            }
            module_export_declaration(current, unaliased);
            free(unaliased);
            return;
        } else if (import && import->module == NULL && import->file == NULL){
            module_export_declaration(current, sep);
            return;
        }
    }
    if (m->is_write) fprintf(m->out,"%s",line);
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
    module * m = &modules[fileno(current)];

    if (add > 0 && type == 0 && c_stmt.type == EXPORT && level[1] == 0){
        c_stmt.export.is_function = 1;
        if (c_stmt.export.name == NULL){
            c_stmt.export.name = c_stmt.export.last_id;
        }

    }

    level[type] += add;
    if (add == 1 && type == 1 && c_stmt.type == EXPORT && c_stmt.export.is_function && level[1] == 1){
        level[1] = 0;
        return module_export_try_end(current, line);
    }
    if (add > 0 && type == 1 && c_stmt.type == EXPORT && c_stmt.export.num_tokens == 0){
        c_stmt.export.type = BLOCK;
        return;
    } else if (type == 1 && add < 0 && c_stmt.type == EXPORT && c_stmt.export.type == BLOCK && level[1] == 0){
        c_stmt.export.name = malloc(strlen("[block_]") + 5);
        sprintf(c_stmt.export.name, "[block_%d]", HASH_COUNT(m->exports));
        module_export_try_end(current, "");
        return;
    }
    module_export_declaration(current, line);
}

void module_export_start(FILE*current, const char * line){
    /* free accumulated tokens */
    memset(&c_stmt, 0, sizeof(c_stmt));
    c_stmt.type = EXPORT;
    c_stmt.export.num_tokens = 0;
    c_stmt.export.type = 0;

    level[1] = 0;
    level[0] = 0;
    recording = 1;
}

module * export_module(module * m, char * alias){
    import_t * import = NULL;
    HASH_FIND_STR(m->imports, alias, import);
    if (import != NULL){
        return import->module;
    }
    /*fprintf(stderr, "exporting %s\n", alias);*/
    import = malloc(sizeof(import_t));
    import->alias  = alias;
    import->file   = alias;
    import->type   = module_import;
    import->module = module_parse(import->file, m->verbose, m->parse_only);

    write_headers(import->module, m);
    HASH_ADD_STR(m->imports, alias, import);

    char * rel_path = relative(m->abs_path, import->module->header_path);
    size_t declaration_len = strlen("#include \"\"") + strlen(rel_path) + 1;

    export_t * export = malloc(sizeof(export_t));
    export->type        = MODULE;
    export->name        = alias;
    export->key         = alias;
    export->declaration = malloc(declaration_len);

    snprintf(export->declaration, declaration_len, "#include \"%s\"", rel_path); 

    export->declaration[declaration_len-1] = '\0';
    HASH_ADD_STR(m->exports, name , export);
    free(rel_path);

    return import->module;
}

void module_export_exports(module * m, char * alias){
    import_t * import = NULL;
    HASH_FIND_STR(m->imports, alias, import);
    if (import != NULL){
        return;
    }
    /*fprintf(stderr, "exporting %s\n", alias);*/
    import = malloc(sizeof(import_t));
    import->alias  = alias;
    import->file   = alias;
    import->type   = module_import;
    import->module = module_parse(import->file, m->verbose, m->parse_only);

    write_headers(import->module, m);
    HASH_ADD_STR(m->imports, alias, import);


    export_t * e;

    module * dep = import->module;
    for (e = &dep->exports[0]; e != NULL; e = e->hh.next) {
        export_t * passthrough = malloc(sizeof(export_t));
        memcpy(passthrough, e, sizeof(export_t));

        passthrough->type        = MODULE;
        passthrough->key         = dep->name;
        passthrough->declaration = strdup("");

        HASH_ADD_STR(m->exports, name, passthrough);
    }
}

void module_export_module(FILE*current, const char * line){
    module * m = &modules[fileno(current)];

    const char * end = line + strlen(line);
    while(end > line && *end != '"') end --;

    const char * module = line + strlen("export");
    while(module < end && isspace(*module)) module++;
    module += strlen("*");
    while(module < end && isspace(*module)) module++;
    module += strlen("from");
    while(module < end && isspace(*module)) module++;
    module++;

    char * alias = (char*)malloc(end - module + 1);
    strncpy(alias, module, end - module);
    alias[end - module] = '\0';

    module_export_exports(m, alias);
}


void module_export_try_end(FILE*current, const char * line){
    module * m = &modules[fileno(current)];

    if (recording && level[1] == 0 && c_stmt.type == EXPORT){
        if (c_stmt.export.name == NULL){
            c_stmt.export.name = c_stmt.export.last_id;
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
        if(m->is_write && c_stmt.export.is_extern == 0) fprintf(m->out,"%s%s",e->declaration, line);
        if (e->type != BLOCK){
            strcat(e->declaration, ";");
        }
        HASH_ADD_STR(m->exports, name, e);
        free(c_stmt.export.tokens);
        c_stmt.export.num_tokens = 0;
        c_stmt.export.tokens = NULL;
        c_stmt.type = NONE;
    } else {
        module_export_declaration(current, line);
    }


}

void module_ID(FILE*current, const char * line){
    if (recording && c_stmt.type == EXPORT){
        /*fprintf(stderr, "\nID: %s, (%lu, %lu)\n", line, c_stmt.export.num_tokens, c_stmt.export.tokens_length);*/
        /*printf("%s", line);*/
        char * token = module_prefix(current, line);
        c_stmt.export.num_tokens++;
        c_stmt.export.tokens = realloc(c_stmt.export.tokens, c_stmt.export.num_tokens * sizeof(char*));
        char * t = strdup(token);
        c_stmt.export.tokens[c_stmt.export.num_tokens - 1] = t;
        c_stmt.export.last_id = t;

        if (
                c_stmt.export.name == NULL &&
                (c_stmt.export.type == STRUCT || c_stmt.export.type == ENUM) 
           ){
            c_stmt.export.name = t;
        }
        c_stmt.export.tokens_length += strlen(token);
        /*fprintf(stderr, "ID: %s\n", token);*/
        free(token);
    } else {
        free(module_prefix(current, line));
    }
}

void module_export_extern(FILE* current, const char * line){
    if (recording){
        if (c_stmt.type == EXPORT){
            c_stmt.export.is_extern = 1;
        }
    }
    module_export_declaration(current, line);
}

void module_export_type(FILE* current, const char * line, enum export_type type){
    if (recording){
        if (c_stmt.type == EXPORT){
            if (c_stmt.export.type == 0){
                c_stmt.export.type = type;
            } else if (c_stmt.export.type == TYPE && c_stmt.export.typedef_type == 0){
                c_stmt.export.typedef_type = type;
            }
        }
    }
    module_export_declaration(current, line);
}

void module_export_declaration(FILE* current, const char * line){
    /*printf("%s", line);*/
    if (recording){
        if (c_stmt.type == EXPORT){
            c_stmt.export.num_tokens++;
            c_stmt.export.tokens = realloc(c_stmt.export.tokens, c_stmt.export.num_tokens * sizeof(char*));
            c_stmt.export.tokens[c_stmt.export.num_tokens - 1] = strdup(line);
            c_stmt.export.tokens_length += strlen(line);
        }
    } else {
        module * m = &modules[fileno(current)];
        if(m->is_write) fprintf(m->out, "%s", line);
    }
}


void module_build_variable(FILE*current, const char * line, enum variable_type type){
    module * m = &modules[fileno(current)];
    struct variable * v = malloc(sizeof(struct variable));
    const char * end = line + strlen(line);

    const char * name = line;
    const char * name_end;
    name += strlen("build");
    while(name < end && isspace(*name)) name++;
    switch (type){
        case set:
            name += strlen("set");
            break;
        case append:
            name += strlen("append");
            break;
        case set_default:
            name += strlen("set");
            while(name < end && isspace(*name)) name++;
            name += strlen("default");
            break;
        default:
            name_end = name;
            while(name_end < end && !isspace(*name_end)) name_end++;
            fprintf(stderr, "WARNING: Unknown build option '%.*s'\n", (int)(name_end - name), name);
            fprintf(stderr, "Expected one of 'set', 'set default', 'depends' or 'append'\n");
            return;
    }
    while(name < end && isspace(*name)) name++;
    name_end = name;
    while(name_end < end && !isspace(*name_end)) name_end++;
    
    v->name = malloc(name_end - name + 1);
    strncpy(v->name, name, name_end - name);
    v->name[name_end - name] = '\0';

    while(end > line && *end != '"') end--;

    const char * value = end - 1;
    while(value > line && *value != '"') value--;
    value++;

    v->value = malloc(end - value + 1);
    strncpy(v->value, value, end - value);
    v->value[end - value] = '\0';
    v->type = type;

    HASH_ADD_STR(m->variables, name, v);
}

void module_build_depends(FILE*current, const char * line){
    module * m = &modules[fileno(current)];

    const char * filename     = line + strlen("build");
    const char * filename_end = line + strlen(line) - 1;

    while(isspace(*filename)) filename++;
    filename += strlen("depends");

    while(filename < filename_end && *filename != '"') filename++;
    filename++;
    unsigned module_length = 0;

    while(filename_end > filename && (*filename_end != '"')) filename_end--;
    size_t length = filename_end - filename;

    char * local = (char*)malloc(length + 1);
    strncpy(local, filename, length);
    local[length] = '\0';

    char * abs_path = realpath(local, NULL);

    module * dependency = NULL;
    HASH_FIND_STR(module_cache, abs_path, dependency);

    if (dependency == NULL){
        dependency = malloc(sizeof(module));

        dependency->abs_path    = abs_path;
        dependency->header_path = NULL;
        dependency->is_exported = false;
        HASH_ADD_STR(module_cache, abs_path, dependency);
    }

    import_t * import = malloc(sizeof(import_t));
    import->type   = c_file;
    import->alias  = abs_path;
    import->file   = abs_path;
    import->module = dependency;

    free(local);

    HASH_ADD_STR(m->imports, alias, import);
}

void module_build_set(FILE*current, const char * line){
    module * m = &modules[fileno(current)];
    struct variable * v = malloc(sizeof(struct variable));
    const char * end = line + strlen(line);

    const char * name = line;
    name += strlen("build");
    while(name < end && isspace(*name)) name++;
    name += strlen("set");
    while(name < end && isspace(*name)) name++;
    const char * name_end = name;
    while(name_end < end && !isspace(*name_end)) name_end++;
    
    v->name = malloc(name_end - name + 1);
    strncpy(v->name, name, name_end - name);
    v->name[name_end - name] = '\0';

    while(end > line && *end != '"') end--;

    const char * value = end - 1;
    while(value > line && *value != '"') value--;
    value++;

    v->value = malloc(end - value + 1);
    strncpy(v->value, value, end - value);
    v->value[end - value] = '\0';

    HASH_ADD_STR(m->variables, name, v);
}


void module_export_fp(FILE* current, const char * line){
    if (recording){
        /*printf("%s: type = %s\n", line, type_names[c_stmt.export.type]);*/
        if (
            c_stmt.type == EXPORT && 
            (
             c_stmt.export.type == VARIABLE ||
             (
                 c_stmt.export.type == TYPE &&
                 c_stmt.export.typedef_type == VARIABLE
             )
            ) && 
            c_stmt.export.name == NULL
        ){
            // determine name
            const char * e = line + strlen(line);
            const char * end = e - 1;
            while(end > line && isspace(*end)) end--;
            while(end > line && *end == ')'  ) end--;
            while(end > line && isspace(*end)) end--;
            end++;

            const char * start = line;
            while (start < end && *start == '('  ) start++;
            while (start < end && isspace(*start)) start++;
            while (start < end && *start == '*'  ) start++;
            while (start < end && isspace(*start)) start++;

            char * s1 = malloc(start - line + 1);
            strncpy(s1, line, start - line);
            s1[start - line] = '\0';

            char * id = malloc(end - start + 1);
            strncpy(id, start, end - start);
            id[end - start] = '\0';

            char * s2 = malloc(e - end + 1);
            strncpy(s2, end, e - end);
            s2[e - end] = '\0';

            c_stmt.export.name = id;
            c_stmt.export.num_tokens += 3;
            c_stmt.export.tokens = realloc(c_stmt.export.tokens, c_stmt.export.num_tokens * sizeof(char*));
            c_stmt.export.tokens[c_stmt.export.num_tokens - 3] = s1;
            c_stmt.export.tokens[c_stmt.export.num_tokens - 2] = id;
            c_stmt.export.tokens[c_stmt.export.num_tokens - 1] = s2;
            c_stmt.export.tokens_length += strlen(line);
            return;
        }
    }

    module_export_declaration(current, line);
}


void module_platform_start(FILE*current, const char * line) {
    size_t length = strlen(line);
    size_t id_len = 0;

    const char * c = line + strlen("platform");
    const char * end = &line[length - 1];

    while(c          < end &&  isspace(*c)       ) c++;
    while(&c[id_len] < end && !isspace(c[id_len])) id_len++;
    
    char * id = malloc(id_len + 1);
    strncpy(id, c, id_len);
    id[id_len] = 0;
}

void module_platform_finish(FILE*current, const char * line) {

}
