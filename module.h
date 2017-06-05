#ifndef __MODULE_H_
#define __MODULE_H_

#include <stdio.h>
#include <uthash.h>
#include <stdbool.h>

typedef struct module_t module;

typedef struct {
    UT_hash_handle hh;
    char * alias;
    char * file;
    module * module;
} import_t;

enum export_type {
    VARIABLE = 0,
    TYPE,
    UNION,
    STRUCT,
    ENUM,
};
typedef struct {
    UT_hash_handle hh;
    char * key;
    char * name;
    enum export_type type;
    char * declaration;
} export_t;

struct variable {
    UT_hash_handle hh;
    char * name;
    char * value;
};

struct module_t {
    UT_hash_handle hh;
    import_t * imports;
    export_t * exports;
    char * name;
    char * abs_path;
    FILE * out;
    char * generated_path;
    char * header_path;
    struct variable * variables;
    bool verbose;
    bool is_exported;
};

enum statement {
    EXPORT = 1 
};

typedef struct{
    enum statement type;
    union {
        struct{
            char * key;
            char * name;
            char * declaration;
            char ** tokens;
            size_t num_tokens;
            size_t tokens_length;
            int is_function;
            int is_struct;
            enum export_type type;
        } export;
    };
} statement_t;

extern module modules[255]; // modules accessed by filenumber
extern int recording;
extern export_t current_export;
extern statement_t c_stmt;

void module_imports(FILE* current, const char * line);
void module_unalias(FILE* current, const char * line);
void module_name(FILE*current, const char * line);
void module_exports(FILE* current, const char * line, enum export_type type);
void module_export_type(FILE* current, const char * line, enum export_type type);
void module_export_start(FILE* current, const char * line);
char * module_prefix (FILE* current, const char * line);
void module_export_try_end(FILE*current, const char * line);

void module_count(FILE*current,int, int, const char * line);
void module_ID(FILE*current, const char * line);
void module_export_declaration(FILE* current, const char * line);

void module_build_set(FILE*current, const char * line);

module * module_parse(const char * filename, bool verbose);

#endif
