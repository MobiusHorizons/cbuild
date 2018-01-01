#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static char escape(char * c) {
  switch(*c){
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'v':
      return '\v';
    default:
      return *c;
  }
}

export char * parse(char * input) {
  char * c = input;
  char * o = input;


  if (*c != '"') {
    return NULL; // nota quoted string
  }
  c++;

  while(*c != 0 && *c != '"'){
    switch(*c) {
      case '\\':
        c++;
        *o = escape(c);
        break;
      default:
        *o = *c;
        break;
    }
    c++; o++;
  }
  *o = 0;
  return input;
}
