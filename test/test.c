

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include "../parser/colors.h"






#include "../deps/hash/hash.h"

#include "../package/index.h"
#include "../package/package.h"
#include "../package/export.h"
#include "string-stream.h"
#include "../deps/stream/stream.h"

#define LEN(array) (sizeof(array)/sizeof(array[0]))

struct test_case_s;
typedef bool (*test_fn)(package_t * pkg, struct test_case_s c, char * out, char ** error);

typedef struct test_case_s {
  char * name;
  char * desc;
  char * input;
  char * output;
  int errors;
  test_fn fn;
  void * data;
} test_case;

typedef struct {
  size_t total;
  size_t passed;
} results_t;

static char * cwd;

static bool check_pkg_name(package_t * pkg, struct test_case_s c, char * out, char ** error) {
  char * desired_name = (char *) c.data;
  bool passed = strcmp(pkg->name, desired_name) == 0;
  if (!passed) {
    asprintf(error, "Incorrect Package Name: Got '%s' expected '%s'\n", pkg->name, desired_name);
  }
  return passed;
}

static test_case package[] = {
  {
    .name   = "package_name.module.c",
    .desc   = "It should choose the correct default package name",
    .input  = "export struct a { int a; };",
    .output = "struct package_name_a { int a; };",
    .fn     = check_pkg_name,
    .data   = "package_name",
    .errors = 0,
  },
  {
    .name   = "package_name.module.c",
    .desc   = "It should update the package name",
    .input  = "package \"test\";export struct a { int a; };",
    .output = "struct test_a { int a; };",
    .fn     = check_pkg_name,
    .data   = "test",
    .errors = 0,
  },
};

static bool check_pkg_exports(package_t * pkg, struct test_case_s c, char * out, char ** error) {
  char * should_export = (char *) c.data;
  package_export_t * exp = (package_export_t*) hash_get(pkg->exports, should_export);
  if (exp == NULL) {
    stream_t * buf = string_stream_new_writer();

    hash_each(pkg->exports, {
      package_export_t * exp = (package_export_t*)val;
      stream_printf(buf, "\t%s: '%s',\n", exp->export_name, exp->symbol);
    });

    asprintf(error, "package '%s' did not export the expected symbol: '%s'.\n"
                    "the following symbols are exported:\n%s",
                    pkg->name, should_export, string_stream_get_buffer(buf));
    stream_close(buf);
    return false;
  }
  return true;
}

static test_case exports[] = {
  {
    .name   = "export.module.c",
    .desc   = "It should export an int",
    .input  = "export int a;",
    .output = "int export_a;",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export an aliased int",
    .input  = "export int a as b;",
    .output = "int export_b;",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export  a function",
    .input  = "export int a(int a1, int a2){}",
    .output = "int export_a(int a1, int a2){}",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should preserve whitespace when exporting a function",
    .input  = "export int  a   (    int      a1      ,       int        a2)         {          }",
    .output = "int  export_a   (    int      a1      ,       int        a2)         {          }",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export an aliased function",
    .input  = "export int a(int a1, int a2) as b{}",
    .output = "int export_b(int a1, int a2){}",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export an enum",
    .input  = "export enum a { a, b, c, d, e, f };",
    .output = "enum export_a { a, b, c, d, e, f };",
    .fn     = check_pkg_exports,
    .data   = "a",
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export an enum with a default value",
    .input  = "export enum a { a = 0, b, c, d, e, f };",
    .output = "enum export_a { a = 0, b, c, d, e, f };",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export an aliased enum",
    .input  = "export enum a { a, b, c, d, e, f } as b;",
    .output = "enum export_b { a, b, c, d, e, f };",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export  a union",
    .input  = "export union a { int i; char c; void * v; };",
    .output = "union export_a { int i; char c; void * v; };",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export an aliased union",
    .input  = "export union a { int i; char c; void * v; } as b;",
    .output = "union export_b { int i; char c; void * v; };",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export  a struct",
    .input  = "export struct a { int a; };",
    .output = "struct export_a { int a; };",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export an aliased struct",
    .input  = "export struct a { int a; } as b;",
    .output = "struct export_b { int a; };",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export a static array of char",
    .input  = "export static const char * names[] = { \"1\" };",
    .output = "static const char * export_names[] = { \"1\" };",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export  a typedef of function pointer",
    .input  = "export typedef int (*a)(int a1, int a2);",
    .output = "typedef int (*export_a)(int a1, int a2);",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export an aliased typedef of function pointer",
    .input  = "export typedef int (*a)(int a1, int a2) as b;",
    .output = "typedef int (*export_b)(int a1, int a2);",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export  a typedef of anonymous struct",
    .input  = "export typedef struct { int a; } b;",
    .output = "typedef struct { int a; } export_b;",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export  a typedef of named struct",
    .input  = "export typedef struct a { int a; } b;",
    .output = "typedef struct a { int a; } export_b;",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export an aliased typedef of anonymous struct",
    .input  = "export typedef struct { int a; } b as c;",
    .output = "typedef struct { int a; } export_c;",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export an aliased typedef of named struct",
    .input  = "export typedef struct a { int a; } b as c;",
    .output = "typedef struct a { int a; } export_c;",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "export.module.c",
    .desc   = "It should export passthrough",
    .input  = "export * from \"example.module.c\";",
    .output = "#include \"example.h\"",
    .fn     = check_pkg_exports,
    .data   = "test2",
    .errors = 0,
  },
};


static test_case symbols[] = {
  {
    .name   = "symbol.module.c",
    .desc   = "It should replace symbols from the current package in a declaration",
    .input  = "export typedef struct { int a; } a_t;a_t b;",
    .output = "typedef struct { int a; } symbol_a_t;symbol_a_t b;",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "symbol.module.c",
    .desc   = "It should replace symbols from the current package in a function type",
    .input  = "export typedef struct { int a; } a_t;a_t b();",
    .output = "typedef struct { int a; } symbol_a_t;symbol_a_t b();",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "symbol.module.c",
    .desc   = "It should replace symbols from the current package in function arguments",
    .input  = "export typedef struct { int a; } a_t;void b(a_t c);",
    .output = "typedef struct { int a; } symbol_a_t;void b(symbol_a_t c);",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "symbol.module.c",
    .desc   = "It should replace symbols from the current package in an exported function type",
    .input  = "export typedef struct { int a; } a_t;export a_t b();",
    .output = "typedef struct { int a; } symbol_a_t;symbol_a_t symbol_b();",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "symbol.module.c",
    .desc   = "It should replace symbols from the current package in exported function arguments",
    .input  = "export typedef struct { int a; } a_t;export void b(a_t c);",
    .output = "typedef struct { int a; } symbol_a_t;void symbol_b(symbol_a_t c);",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "symbol.module.c",
    .desc   = "It should replace enum symbols from the current package in a declaration",
    .input  = "export enum a { a = 0 };enum a b;",
    .output = "enum symbol_a { a = 0 };enum symbol_a b;",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "symbol.module.c",
    .desc   = "It should replace union symbols from the current package in a declaration",
    .input  = "export union a { int a; };union a b;",
    .output = "union symbol_a { int a; };union symbol_a b;",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "symbol.module.c",
    .desc   = "It should replace struct symbols from the current package in a declaration",
    .input  = "export struct a { int a; };struct a b;",
    .output = "struct symbol_a { int a; };struct symbol_a b;",
    .fn     = NULL,
    .errors = 0,
  },
  {
    .name   = "symbol.module.c",
    .desc   = "It should replace typed symbols from the current package in a struct body",
    .input  = "export typedef struct { int a; } a;export struct c { a b; };",
    .output = "typedef struct { int a; } symbol_a;struct symbol_c { symbol_a b; };",
    .fn     = NULL,
    .errors = 0,
  },
};

static test_case _imports[] = {
  {
    .name   = "import.module.c",
    .desc   = "It should import other modules",
    .input  = "import example from \"example.module.c\";",
    .output = "#include \"example.h\"",
    .fn     = NULL,
    .errors = 0,
  },
};

static bool run_test(test_case c) {
  printf(BOLD "  %s: \r" RESET, c.desc); fflush(stdout);
  stream_t * in  = string_stream_new_reader(c.input);
  stream_t * out = string_stream_new_writer();
  char * error = NULL;
  char * fn_err = NULL;
  char * key = NULL;

  asprintf(&key, "%s/%s", cwd, c.name);
  char * generated = index_generated_name(key);
  package_t * p = index_parse(in, out, c.name, key, generated, &error, false, false);

  char * buf = string_stream_get_buffer(out);
  bool desired_output = buf && strcmp(buf, c.output) == 0;
  bool function_test  = c.fn ? c.fn(p, c, buf, &fn_err) : true;

  if (error) {
    printf(RED    "%s\n" RESET, error);
  }
  if ( desired_output && function_test) {
    printf(GREEN "✓ " RESET BOLD "%s: \n" RESET, c.desc); fflush(stdout);
    stream_close(out);
    return true;
  }

  printf(RED "✕ " RESET BOLD "%s: \n\n" RESET, c.desc); fflush(stdout);

  if (!function_test && fn_err != NULL) {
    printf(RED    "%s\n\n" RESET, fn_err);
  }

  if (! desired_output) {
    printf(YELLOW "Input  : '%s'\n" RESET, c.input);
    printf(RED    "Output : '%s'\n" RESET, buf);
    printf(GREEN  "Expect : '%s'\n" RESET, c.output);
  }

  printf("\n");
  stream_close(out);
  return false;
}

results_t run_tests(char * group_name, test_case cases[], size_t length) {
  int i;
  size_t passed = 0;
  printf(BOLD "\n=== Test group " UNDERLINE "%s" RESET BOLD " ===\n\n" RESET, group_name);
  for ( i = 0; i < length; i++) {
    if (run_test(cases[i])) passed ++;
  }
  results_t r = {
    .total  = length,
    .passed = passed,
  };
  printf("%s", passed == length ? GREEN : RED);
  printf("\n[%s] (%lu/%lu) tests passed\n" RESET, group_name, passed, length);
  return r;
}

results_t combine_results(results_t a, results_t b) {
  a.total  += b.total;
  a.passed += b.passed;
  return a;
}

int main() {
  cwd = getcwd(NULL, 0);
  results_t r = {0};
  r = combine_results(run_tests("package", package, LEN(package)), r);
  r = combine_results(run_tests("exports", exports, LEN(exports)), r);
  r = combine_results(run_tests("symbols", symbols, LEN(symbols)), r);
  r = combine_results(run_tests("imports", _imports, LEN(_imports)), r);

  printf("%s", r.passed == r.total ? GREEN : RED);
  printf("[all tests] (%lu/%lu) tests passed\n" RESET, r.passed, r.total);
  return r.passed == r.total ? 0 : 1;
}