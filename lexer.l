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
%%
{ID} {
    module_prefix(yyin,yytext);
}

{ID}"."{ID} {
    module_unalias(yyin, yytext);
}

({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+	{ 
  printf("%s",yytext);
}

import{SEP}{ID}{SEP}from{SEP}{FILENAME}{SEP}?; {
  module_imports(yyin, yytext);
}

export{SEP}{ID}{SEP}?; {
  module_exports(yyin, yytext);
}

%%
