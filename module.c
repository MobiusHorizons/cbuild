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
        printf("'%s' has already been imported from %s\n", alias, import->file);
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
    HASH_ADD_STR(m->imports, alias, import);
}
void module_exports(FILE* current, const char * line){
    module * m = &modules[fileno(current)];

    const char * end = line + strlen(line) - 1;
    const char * name = line + strlen("export");

    while(name < end && isspace(*name)) name++;

    if (*end == ';') end--;
    while (end > name && isspace(*end)) end--;

    end++;

    export_t * e = malloc(sizeof(export_t));
    e->name = malloc(end - name + 1);
    strncpy(e->name, name, end - name);
    e->name[end-name] = '\0';
    HASH_ADD_STR(m->exports, name, e);
}

void module_prefix(FILE* current, const char * line){
    module * m = &modules[fileno(current)];

    if (m->exports){
        export_t * e = NULL;
        HASH_FIND_STR(m->exports, line, e);
        if (e && m->name){
            printf("%s_%s", m->name, line);
            return;
        }
    }
    
    printf("%s", line);
}

module * module_parse(const char * filename){
    yyscan_t scanner;
    yylex_init(&scanner);

    char buffer[2][PATH_MAX];

    char * cwd = getcwd(buffer[0], PATH_MAX);

    /*printf("CWD = %s\n", cwd);*/
    int fd = open(filename, O_RDONLY);
    if (fd < 0){
        fprintf(stderr, "Error parsing %s\n", filename);
        exit(-1);
    }
    
    // set the current directory to the parent directory of the open file
    char * new_cwd = dirname_r(filename, buffer[1]);
    /*printf("CD: %s\n", new_cwd);*/
    chdir(new_cwd);

    /*printf("parsing %s, fd = %d\n", filename, fd);*/
    char * fname = strdup(filename);
    modules[fd].imports = NULL;

    // get full path for caching purposes
    modules[fd].abs_path = realpath(filename, NULL);


    char * package = basename(fname);
    char * ext = rindex(package, '.');
    if (ext){
        *ext = '\0';
    }
    modules[fd].name = strdup(package);
    yyset_in(fdopen(fd, "rb"), scanner);
    yylex(scanner);
    yylex_destroy(scanner);

    /*printf("CD: %s\n", cwd);*/
    chdir(cwd); // return to where we were before

    module * m = malloc(sizeof(module));
    memcpy(m, &modules[fd], sizeof(module));

    free(fname);
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
            printf("%s_%s", import->module->name, sep);
            return;
        }
    }
    printf("%s",line);
}
