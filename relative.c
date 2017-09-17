#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define SEP '/'

int count_levels(const char * path){
  int i = 0, levels = 0;
  bool escaped = false;

  while(path[i] != '\0'){
    if (escaped) {
      escaped = false;
      continue;
    }
    if (path[i] == '\\') escaped = true;
    if (path[i] == SEP) levels++;
    i++;
  }

  return levels;
}

char * relative(const char * from, const char * to){
  // determine shared prefix;

  int i = 0, last_sep = 0;
  while(from[i] != '\0' && to[i] != '\0' && from[i] == to[i]){
    if (from[i] == SEP){
      last_sep = i;
    }
    i++;
  }
  int dots = count_levels(&from[last_sep +1]);

  char * rel = malloc((dots == 0 ? 2 : (dots * 3)) + strlen(&to[last_sep]) + 1);
  rel[0] = 0;
  if (dots == 0){
      strcpy(rel, "./");
  }
  while (dots--){
    strcat(rel, "../");
  }
  strcat(rel, &to[last_sep + 1]);
  return rel;
}


