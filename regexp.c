#include <stdio.h>
#include <string.h>

char* regexp_hlp(char* s, char* re, char* mod, char* start, char* dore) {
  fprintf(stderr, "  == >%-20s< >%-20s< >%-20s< >%-20s< >%-20s<\n", s, re, mod, start, dore);
  if (!s || !re) return NULL;
  if (!*re) return start;
  if (*re == '$') return *s ? NULL : start;
  if (!*s && *re == '*') return *(re+1) ? NULL : start;
  if (!*s && *(re+1) == '*' && !*(re+2)) return start;
  if (!*s) return NULL;
  if (*re == '^') return regexp_hlp(s, re+1, mod, start, NULL); // disable backtrack
  if (*re == '.') return regexp_hlp(s+1, re+1, mod, start, dore);
  if (*re == '*') return regexp_hlp(s, re+1, mod, start, dore);
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

int test(char* s, char* re, char* mod, char* expect) {
  char* x = regexp(s, re, NULL);
  int r = (expect == x);
  if (!r && expect && x) r = (strcmp(expect, x) == 0);
  printf("%s ---- test >%s< >%s< => >%s< got >%s<\n", r ? "OK" : "ERROR", s, re, expect, x);
  return r ? 0 : 1;
}

void main() {
  // bAD!!! printf("MATCH: %s\n", regexp("ababcabcdabcde", "...............", NULL));
  int fails = 0
    + test("a", "", NULL, "a")
    + test("a", "a", NULL, "a")
    + test("a", "x", NULL, NULL)
    + test("aa", "aa", NULL, "aa")
    + test("aa", "aaa", NULL, NULL)
    + test("ab", "a", NULL, "ab")
    + test("ab", "b", NULL, "b")
    + test("aba", "b", NULL, "ba")
    + test("aba", "ba", NULL, "ba")
    + test("aababc", "ab", NULL, "ababc")
    + test("aababc", "abc", NULL, "abc")
    + test("aababcabcd", "abcd", NULL, "abcd")
    + test("aababcabcd", "abcd", NULL, "abcd")

    + test("a", ".", NULL, "a")
    + test("a", "..", NULL, NULL)
    + test("abba", "....", NULL, "abba")
    + test("ababba", "....", NULL, "ababba")
    + test("ababba", "abba", NULL, "abba")

    + test("a", "^a", NULL, "a")
    + test("b", "^a", NULL, NULL)
    + test("ba", "^ba", NULL, "ba")
    + test("ba", "^b", NULL, "ba")
    + test("ba", "^a", NULL, NULL)

    + test("a", "a$", NULL, "a")
    + test("aa", "a$", NULL, "a")
    + test("aaa", "a$", NULL, "a")
    + test("axa", "a$", NULL, "a")

    + test("a", "^aa$", NULL, NULL)
    + test("aa", "^aa$", NULL, "aa")
    + test("aaa", "^aa$", NULL, NULL)

    + test("", "a*", NULL, "")
    + test("a", "a*", NULL, "a")
    + test("b", "a*", NULL, "b")
    + test("aa", "a*", NULL, "aa")
    + test("aa", "aa*", NULL, "aa")
    + test("a", "aa*", NULL, "a")
    + test("", "aa*", NULL, NULL)

    + test("abba", "a*bba*", NULL, "abba")
    + test("bba", "a*bba*", NULL, "bba")
    + test("bb", "a*bba*", NULL, "bb")
    + test("abba", "ba*b", NULL, "bba")
    + test("abba", "^abba", NULL, "abba")
    + test("abba", "^a*bba", NULL, "abba")
    + test("abba", "^a*bba*", NULL, "abba")
    
    ;
  printf("FAILS %d\n", fails);
}
