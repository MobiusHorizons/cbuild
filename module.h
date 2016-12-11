#ifndef __MODULE_H_
#define __MODULE_H_

#include <stdio.h>
#include <uthash.h>

typedef struct module_t module;

typedef struct {
    UT_hash_handle hh;
    char * alias;
    char * file;
    module * module;
} import_t;

typedef struct {
    UT_hash_handle hh;
    char * name;

} export_t;

struct module_t {
    UT_hash_handle hh;
    import_t * imports;
    export_t * exports;
    char * name;
    char * abs_path;
};

extern module modules[255]; // modules accessed by filenumber

void module_imports(FILE* current, const char * line);
void module_unalias(FILE* current, const char * line);
void module_exports(FILE* current, const char * line);
void module_prefix (FILE* current, const char * line);

module * module_parse(const char * filename);

#endif
