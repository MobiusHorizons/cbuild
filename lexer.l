%{
  #include "./module.h"
%}

%option reentrant
%option noyywrap


DIGIT    [0-9]
ID	     [a-zA-Z_][a-zA-Z0-9_]*
FILENAME \"[a-zA-Z0-9\/.-]+\"
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

"."{ID} {
    module_export_declaration(yyin,yytext);
}

{ID}"."{ID} {
    module_unalias(yyin, yytext);
}
package{SEP}({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+{SEP}?;	{ 
    module_name(yyin, yytext);
}

({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+	{ 
  fprintf(yyout, "%s",yytext);
}

import{SEP}{ID}{SEP}from{SEP}{FILENAME}{SEP}?; {
  module_imports(yyin, yytext);
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

export{SEP}? {
  module_export_start(yyin, yytext);
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
