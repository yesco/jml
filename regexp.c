#include <stdio.h>
#include <string.h>

// LOL use this instead
// http://stackoverflow.com/questions/4040835/how-do-i-write-a-simple-regular-expression-pattern-matching-function-in-c-or-c
// https://swtch.com/~rsc/regexp/regexp1.html

/* match: search for regexp anywhere in text */

typedef struct Match {
  char* start;
  char* end;
} Match;

Match match(char *regexp, char *text) {
  #define MAXLEVELS 10
  Match m[MAXLEVELS] = { 0 };
  
  // can't do forward declaration for "dynamic function"
  int (*matchstar)(int c, char *regexp, char *text, int level);
  
  /* matchhere: search for regexp at beginning of text */
  int Xmatchhere(char *regexp, char *text, int level) {
    if (level >= MAXLEVELS) { fprintf(stderr, "{Regexp full %s}", regexp); return 0; }
    if (!*regexp) { m[0].end = text; return 1; }
    if (*regexp == '(') { m[++level].start = text; return Xmatchhere(regexp+1, text, level); }
    if (*regexp == ')') { m[level].end = text; return Xmatchhere(regexp+1, text, level); }
    if (regexp[1] == '*') return matchstar(*regexp, regexp+2, text, level);
    if (*regexp == '$' && !regexp[1]) { if (!m[level].end) m[level].end = text; return *text == '\0'; }
    if (*text && (*regexp == '.' || *regexp == *text))
      return Xmatchhere(regexp+1, text+1, level);
    return 0;
  }

  /* matchstar: search for c*regexp at beginning of text */
  int Xmatchstar(int c, char *regexp, char *text, int level) {
    do { /* a * matches zero or more instances */
      if (Xmatchhere(regexp, text, level)) return 1;
    } while (*text && (*text++ == c || c == '.'));
    return 0;
  }

  int Xmatch(char* regexp, char* text, int level) {
    if (*regexp == '^') return Xmatchhere(regexp+1, m[level].start = text, level);
    do { /* must look even if string is empty */
      if (Xmatchhere(regexp, m[level].start = text, level)) return 1;
    } while (*text++);
    return 0;
  }

  matchstar = Xmatchstar;
  if (Xmatch(regexp, text, 0)) {
    int i;
    for(i = 0; i < MAXLEVELS; i++) {
      char* s = m[i].start && m[i].end ? strndup(m[i].start, m[i].end - m[i].start) : NULL;
      if (!s) break;
      fprintf(stderr, "   => '%s'./%s/ ... %d >%s<\n", text, regexp,
              i, s);
    }
    return m[0];
  } else {
    m[0] = (Match){0, 0};
    return m[0];
  }
  #undef MAXLEVELS    
}

// this looks messier, but returns pointer to start of match

char* regexp(char* s, char* re, char* mod) {

  char* regexp_hlp(char* s, char* re, char* mod, char* start, char* dore) {
    fprintf(stderr, "  == >%-20s< >%-20s< >%-20s< >%-20s< >%-20s<\n", s, re, mod, start, dore);
    if (!s || !re) return NULL;
    if (!*re) return start;
  
    if (*re == '$') return *s ? NULL : start;
    if (*re == '^') return regexp_hlp(s, re+1, mod, start, NULL); // disable backtrack TODO: must be breakable???

    if (*re == '*') return regexp_hlp(s, re+1, mod, start, dore);
    if (*s && (*s == *re || *re == '.')) {
      char* x = regexp_hlp(s+1, re+1, mod, start, dore);
      if (x) return x;
    }
    if (*(re+1) == '*') return regexp_hlp(s, re+2, mod, start, dore);
    if (!*s) return NULL;
    // backtrack
    return regexp_hlp(start+1, dore, mod, start+1, dore);
  }

  return regexp_hlp(s, re, mod, s, re);
}

int test(char* s, char* re, char* mod, char* expect) {
  int r;
  if (1) {
    Match m = match(re, s);
    int v = m.start ? 1 : 0;
    r = v == (expect ? 1 : 0);
    char* rr = strndup(m.start, m.end-m.start);
    printf("%s ---- test >%s< >%s< => >%s< got >%s< result=%d\n", r ? "OK" : "ERROR", s, re, expect, rr, v);
    return r ? 0 : 1;
  } else {  
    char* x = regexp(s, re, NULL);
    int r = (expect == x);
    if (!r && expect && x) r = (strcmp(expect, x) == 0);
    printf("%s ---- test >%s< >%s< => >%s< got >%s<\n", r ? "OK" : "ERROR", s, re, expect, x);
    return r ? 0 : 1;
  }
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

    // backtrack on greedy... ???
    + test("abba", "a*a", NULL, "a")
    + test("abba", "a*a*a", NULL, "a")
    + test("abba", "a*a*a*a", NULL, "a")
    + test("abba", "a*a*a*a*a", NULL, "a")
    + test("abba", "^a*b*b*a*$", NULL, "abba")

    + test("<xml><foo>bar</foo></xml>", "<(foo)>(.*)<(/foo)>", NULL, "abba")
    ;
  printf("FAILS %d\n", fails);
}
