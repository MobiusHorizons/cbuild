#include "./module.h"

int main( argc, argv )
int argc;
char **argv;
{
  ++argv, --argc;	/* skip over program name */
  if ( argc > 0 )
      module_parse(argv[0]);
}

