#define _BSD_SOURCE
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>

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

char * utils_relative(const char * from, const char * to){
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
  /*if (dots == 0){*/
      /*strcpy(rel, "./");*/
  /*}*/
  while (dots--){
    strcat(rel, "../");
  }
  strcat(rel, &to[last_sep + 1]);
  return rel;
}

bool utils_newer(const char * a, const char * b) {
  struct stat sta;
  struct stat stb;

  int resa = lstat(a, &sta);
  if (resa == -1) return false;

  int resb = lstat(b, &stb);
  if (resb == -1) return true;


#ifdef __MACH__
  if (sta.st_mtimespec.tv_sec == stb.st_mtimespec.tv_sec) {
    return sta.st_mtimespec.tv_nsec > stb.st_mtimespec.tv_nsec;
  }
  return sta.st_mtimespec.tv_sec > stb.st_mtimespec.tv_sec;
#else
  if (sta.st_mtim.tv_sec == stb.st_mtim.tv_sec) {
    return sta.st_mtim.tv_nsec > stb.st_mtim.tv_nsec;
  }
  return sta.st_mtim.tv_sec > stb.st_mtim.tv_sec;
#endif
}