%{
  #include "./module.h"
  #define YY_USER_ACTION CONTEXT()
%}

%option reentrant
%option noyywrap
%option yylineno


DIGIT    [0-9]
ID	     [a-zA-Z_][a-zA-Z0-9_]*
FILENAME \"[-a-zA-Z0-9\/._]+\"
SP       (u8|u|U|L)
ES       (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS       [ \t\v\n\f]
SEP      [ \t\v\n\f]+
STRUCT   ((struct|union|enum)?)
TYPE     (typedef|int|char|long|short|double|float|void)
%%
{ID} {
    module_ID(yyin, yytext);
}

"/*"(.*)"*/" {
  module_export_declaration(yyin,yytext);
}

"."{ID} {
    module_export_declaration(yyin,yytext);
}

{ID}"."{ID} {
    module_unalias(yyin, yytext);
}
package{SEP}({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+{SEP}?;	{ 
    module_name(yyin, yytext);
}

build{SEP}depends{SEP}{FILENAME}{SEP}?; {
    module_build_depends(yyin, yytext);
}

build{SEP}set{SEP}default{SEP}{ID}{SEP}\"([^"\\\n]|{ES})*\"; {
    module_build_variable(yyin, yytext, set_default);
}

build{SEP}set{SEP}{ID}{SEP}\"([^"\\\n]|{ES})*\"; {
    module_build_variable(yyin, yytext, set);
}

build{SEP}append{SEP}{ID}{SEP}\"([^"\\\n]|{ES})*\"; {
    module_build_variable(yyin, yytext, append);
}

build{SEP}{ID}[^;\n]+(;|\n) {
    module_build_variable(yyin, yytext, unknown);
}

({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+	{  
  module_export_declaration(yyin,yytext);
}

import{SEP}{ID}{SEP}from{SEP}{FILENAME}{SEP}?; {
  module_imports(yyin, yytext);
}

import[^;\n]+(;|\n) {
  module * m = &modules[fileno(yyin)];
  long length = strlen(yytext);
  if (yytext[length - 1] == '\n') length--;
  fprintf(stderr, "%s:%d:%d: Invalid import syntax\n", m->rel_path, yylineno, yycolumn);
  fprintf(stderr, "%s", yytext);
  fprintf(stderr, "Expecting \"import <ID> from \"<filename>\";\"\n\n");
  exit(1);
}

platform{SEP}{ID}{SEP}?\{ {
  module_platform_start(yyin, yytext);
}

\({SEP}?\*{SEP}?{ID}{SEP}?\) {
  module_export_fp(yyin, yytext);
}

\( {
  module_count(yyin, 1, 0, yytext);
}

\) {
  module_count(yyin, -1, 0, yytext);
}

\{ {
  module_count(yyin, 1, 1, yytext);
}

\} {
  module_count(yyin, -1, 1, yytext);
}

; {
  module_export_try_end(yyin, yytext);
}

{SEP}?as{SEP}{ID} {
  module_export_rename(yyin, yytext);
}

export{SEP}\*{SEP}from{SEP}{FILENAME}{SEP}?; {
  module_export_module(yyin, yytext);
}

export{SEP}? {
  module_export_start(yyin, yytext);
}

extern{SEP}? {
  module_export_extern(yyin, yytext);
}

typedef{SEP}? {
  module_export_type(yyin, yytext, TYPE);
}

enum{SEP}? {
  module_export_type(yyin, yytext, ENUM);
}

struct{SEP}? {
  module_export_type(yyin, yytext, STRUCT);
}

(.|\n) {
  module_export_declaration(yyin,yytext);
}

%%
