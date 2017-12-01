#ifndef __MODULE_H_
#define __MODULE_H_

#include <stdio.h>
#include <uthash/uthash.h>
#include <stdbool.h>

typedef struct module_t module;

struct {
    int line;
    int col;
    int fd;
    const char * text;
} context;

#define CONTEXT() {                                         \
    context.line = YY_CURRENT_BUFFER_LVALUE->yy_bs_lineno;  \
    context.col  = YY_CURRENT_BUFFER_LVALUE->yy_bs_column;  \
    context.fd   = fileno(yyin);                            \
    context.text = yytext;                                  \
};

enum import_type {
    global_import = 0,
    module_import,
    c_file,
    header,
};

typedef struct {
    UT_hash_handle hh;
    enum import_type type;
    char * alias;
    char * file;
    module * module;
} import_t;

enum variable_type {
    set,
    set_default,
    append,
    unknown,
};

enum export_type {
    VARIABLE = 0,
    TYPE,
    UNION,
    STRUCT,
    ENUM,
    MODULE,
    BLOCK,
};

typedef struct {
    UT_hash_handle hh;
    char * prefix;
    char * name;
    char * alias;
    enum export_type type;
    char * declaration;
    bool is_alias;
} export_t;

struct variable {
    UT_hash_handle hh;
    enum variable_type type;
    char * name;
    char * value;
};

struct module_t {
    UT_hash_handle    hh;
    import_t *        imports;
    export_t *        exports;
    char *            name;
    char *            rel_path;
    char *            abs_path;
    FILE *            out;
    char *            generated_path;
    char *            header_path;
    struct variable * variables;
    bool              verbose;
    bool              is_exported;
    bool              is_write;
    int               force;
};

enum statement {
    NONE = 0,
    EXPORT, 
};

typedef struct{
    enum statement type;
    union {
        struct{
            char * key;
            char * name;
            char * declaration;
            char ** tokens;
            char * last_id;
            size_t num_tokens;
            size_t tokens_length;
            int is_function;
            int is_struct;
            int is_extern;
            enum export_type type;
            enum export_type typedef_type;
            char * alias;
        } export;
    };
} statement_t;

extern module modules[255]; // modules accessed by filenumber
extern int recording;
extern export_t current_export;
extern statement_t c_stmt;

void enumerate_exports(module * m, FILE* out);
void module_imports(FILE* current, const char * line);
void module_unalias(FILE* current, const char * line);
void module_name(FILE*current, const char * line);
void module_exports(FILE* current, const char * line, enum export_type type);
void module_export_type(FILE* current, const char * line, enum export_type type);
void module_export_extern(FILE* current, const char * line);
void module_export_fp(FILE* current, const char * line);
void module_export_start(FILE* current, const char * line);
void module_export_module(FILE* current, const char * line);
void module_export_rename(FILE* current, const char * line);
char * module_prefix (FILE* current, const char * line);
void module_export_try_end(FILE*current, const char * line);

void module_count(FILE*current,int, int, const char * line);
void module_ID(FILE*current, const char * line);
void module_export_declaration(FILE* current, const char * line);

void module_platform_start(FILE*current, const char * line);
void module_platform_finish(FILE*current, const char * line);

void module_build_depends(FILE*current, const char * line);
void module_build_variable(FILE*current, const char * line, enum variable_type type);

module * module_parse(const char * filename, bool verbose, int force);

#endif
