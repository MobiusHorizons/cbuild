%{
  #include "./module.h"
%}

%option reentrant


DIGIT    [0-9]
ID	     [a-zA-Z][a-zA-Z0-9]*
FILENAME \"[a-zA-Z0-9\/.-]+\"
SEP      [\t ]+
%%
{ID} {
    module_prefix(yyin,yytext);
}

{ID}"."{ID} {
    module_unalias(yyin, yytext);
}

import{SEP}{ID}{SEP}from{SEP}{FILENAME}{SEP}?; {
  module_imports(yyin, yytext);
}

export{SEP}{ID}{SEP}?; {
  module_exports(yyin, yytext);
}

%%
int yywrap(){
  return 1;
}



