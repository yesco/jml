// generic field extractor for jml
// [x foo DATA] => value
//
// DATA formats:
//   <crap><foo>value</foo></crap>
//   { foo: "value" }
//   foo: value
//   foo=value\nbar=value\n[section]\n
//   foo	value # tabbed
//   first second third fourth
//   [first, second, third...]
//   first\nsecond\nthird\n

// comments ignored
//   ; foo
//   // bar
//   # fie
//   ... # fum

// field spec:
//   foo => just find it, any level
//   foo: => just find it, any level
//   /foo => top level
//   //foo => any level (lol)
//
//   foo.bar => json style
//   foo/bar => xpath style
//   foo..bar => foo.X.bar ok
//   foo//bar => xpath style
//   foo:bar: => outline style
//
// compile:
//   clang -Wwritable-strings xtract.cc && ./a.out

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

char* xresult = NULL;

char* xtract(char* p, char* t, int one, int level) {
  int isXML=0, isJSON=0, isCSV=0, isDict=0, isINI=0, isYAML=0, isFlat=0;
  int lno=0;

  xresult = NULL;
  // TODO: make into callback?
  auto result = [](char* start, char* end)->int {
    if (!start || !end) return 0;
    if (xresult) free(xresult);
    xresult = strndup(start, end-start);
    printf(">>>>>>> ");
    while(*start && start < end) {
      putchar(*start);
      start++;
    }
    putchar('\n');
    return 1;
  };
  // all parse func return 0 if EOF
  auto eof = [&]()->int {
    return !t || !*t;
  };
  auto peek = [&]()->char {
    return eof() ? 0 : *t;
  };
  auto next = [&]()->char {
    if (eof()) return 0;
    t++;
    if (eof()) return 0;
    return *t;
  };
  auto skip = [&](char c)->char {
    if (eof()) return 0;
    while(peek() == c)
      next();
    return 1;
  };
  auto skipspc = [&]()->int {
    // no skip newline
    while(!eof() && (peek()=='\t' || peek()==' '))
      next();
    if (eof()) return 0;
    return 1;
  };
  auto skipalpha = [&]()->int {
    while(!eof() && isalnum(*t))
      t++;
    return !eof();
  };
  auto skiptoname = [&]()->int {
    char* pp = p;
    if (eof()) return 0;
    //if (!*p) ...? match? all?
    // find first matching letter
    char c = peek();
    while(!eof() && c != *pp)
      c = next();
    if (eof()) return 0;

    char q = (skip('"') || skip('\''))? c : 0;
    printf("1.ID:TT '%s'\n", t);
    printf("1.ID:PP '%s'\n", pp);

    if (eof()) return 0;
    while(!eof() && *pp && *pp==*t && isalnum(*pp)) {
      next();
      pp++;
    }
    if (isalnum(*pp) && *t != *pp)
      return 0; // more
    if (q) skip(q);
    if (eof()) return 0;
    if (*pp == ':' || *pp == '/') pp++;
    skipspc();
    // eat's really or but... n/cares
    skip(':'); skip('='); skip('\t');
    skipspc();
    printf("2.ID:TT '%s'\n", t);
    printf("2.ID:PP '%s'\n", pp);
    // ok we have a match
    p = pp;
    return 1;
  };
  auto value = [&]()->int {
    skipspc();
    if (eof()) return 0;
    // get quote character
    char q = *t;
    if (q != '"' && q != '\'') {
      // unquoted
      char* start = t;
      while(!eof() &&
	    !isspace(peek()) &&
	    peek()!=',') {
	next();
      }
      char* afterend = t;
      next();
      return result(start, afterend);
    } else {
      // quoted
      char* start = t;
      while(!eof() && peek()!=q)
	next();
      if (eof()) return 0;
      next();
      char* afterend = t;
      if (peek() == ',') next();
      return result(start, afterend);
    }
  };

  if (!t || !p) return NULL;
  
  // -- determine format type
  if (!skipspc()) return NULL;
  // single tap => CSV?
  isDict = strchr(t, '\t') ? 1 : 0;
  isJSON = (peek() == '{' || peek() == '[') ? 1 : 0;
  // JSONP
  {
    char* savet = t;
    if (skipalpha() && peek() == '(') {
      next();
      isJSON = 1;
    } else {
      // restore
      t = savet;
    }
  }
  // if have 3 commas we say it's CSV, lol
  {
    char* m = strchr(t, ',');
    if (m) {
      m = strchr(m+1, ',');
      if (m) {
	m = strchr(m+1, ',');
	isCSV = m ? 1 : 0;
      }
    }
  }
  // has [section]
  isINI = (strstr(t, "\n[") || strstr(t, "\n&#91;")) ? 1 : 0;
  // indent and ':' TOOD: not good
  isYAML = !isJSON && strstr(t, "\n  ") && strstr(t, ":") ? 1 : 0;
  // many spaces (and not json/yaml)
  isFlat = !isJSON && !isYAML && strstr(t, "    ") ? 1 : 0;
		 
  if (isXML) printf("%% isXML\n");
  if (isJSON) printf("%% isJSON\n");
  if (isCSV) printf("%% isCSV\n");
  if (isDict) printf("%% isDict\n");
  if (isINI) printf("%% isINI\n");
  if (isYAML) printf("%% isYAML\n");
  if (isFlat) printf("%% isFlat\n");

  printf("TT '%s'\n", t);
  printf("PP '%s'\n", p);
  printf("c '%c'\n", peek());

  if (eof()) return NULL;
  
  if (isYAML) {
    do {
      if (skiptoname()) {
	printf("===PP '%s'\n", p);
	skip(':');
	skipspc();
	printf("===TT '%s'\n", t);
	if (value()) return NULL;
      } else {
	// not match just prefix
      }
      // try next
      t++;
    } while(!eof());
  }

  return NULL;
}

int main() {
  char* txt = NULL;
  char* pat = NULL;
  char* res = NULL;
  
  FILE* f = fopen("xtract.tst", "r");
  int len = 1024;
  char line[len];
  while(fgets(line, len, f)) {
    // remove newline
    if (*line)
      line[strlen(line)-1] = 0;

    if (line == strstr(line, "=== ")) {
      // text to test
      if (txt) free(txt);
      txt = strdup(line+4);
      printf("=== %s\n", txt);
    } else if (strstr(line, " =>")) {
      char* l = strdup(line);
      // get pattern => result
      if (pat) free(pat);
      pat = strstr(l, " =>");
      // split before match
      *pat = 0;

      // result is after
      if (res) free(res);
      res = pat+3;
      if (*res == ' ') res++;

      // point to first string
      pat = l;

      // test!
      printf(" EXPACT: %s => %s\n", pat, res);
      if (xresult) free(xresult);
      xresult = NULL;
      char* r = xtract(pat, txt, 1, 0);
      if (xresult && 0==strcmp(xresult, res)) {
	printf("CORRECT: %s => %s\n", pat, xresult);
      } else {
	printf("  ERROR: %s => %s\n", pat, xresult);
      }
    } else if (!*line) {
      // empty line - fine
    } else if (line[0] == '#') {
      // comment
    } else {
      printf("%% Unrecognized line: %s\n", line);
    }
  }
  
  fclose(f);
}
