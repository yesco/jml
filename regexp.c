
#include <stdio.h>

char* regexp_hlp(char* s, char* re, char* mod, char* start, char* dore) {
  fprintf(stderr, "== >%-20s< >%-20s< >%-20s< >%-20s< >%-20s<\n", s, re, mod, start, dore);
  if (!s || !re) return NULL;
  if (!*re) return start;
  if (*re == '$') return *s ? NULL : start;
  if (!*s && *re == '*') return *(re+1) ? NULL : start; // TOOD: Needed?
  if (!*s && *(re+1) == '*' && !*(re+2)) return start;
  if (!*s) return NULL;
  if (*re == '^') return regexp_hlp(s, re+1, mod, start, NULL); // disable backtrack
  if (*re == '.') return regexp_hlp(s+1, re+1, mod, start, dore);
  if (*re == '*') return regexp_hlp(s+1, re-1, mod, start, dore);
  if (*s == *re) {
    char* x = regexp_hlp(s+1, re+1, mod, start, dore);
    if (x) return x;
  } else if (*(re+1) == '*') return regexp_hlp(s, re+2, mod, start, dore);
  // backtrack
  return regexp_hlp(start+1, dore, mod, start+1, dore);
}

char* regexp(char* s, char* re, char* mod) {
  return regexp_hlp(s, re, mod, s, re);
}

void main() {
  // bAD!!! printf("MATCH: %s\n", regexp("ababcabcdabcde", "...............", NULL));
  printf("MATCH: %s\n", regexp("x", "aa*", NULL));
}
