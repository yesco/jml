// A cheating simple "xmlpath-style" parser
// Note: it doesn't care if xml doc is compliant, neither does it validate

#include <stdio.h>
#include <stddef.h>

// xpath test-suite
// https://dev.w3.org/2007/xpath-full-text-10-test-suite/

// returned at error
#define XPERR NULL

// debug it
#define XTRACE 0

// reports content of current element
// returns pointer after result
char* gotit(char* m) {
  char* s = m;
  char* e = m;
  if (!m || !*m) return NULL;
  printf("gotit m='%s'\n", m);
  
  // "balance" tags
  int depth = 1;
  while(*m && depth > 0) {
    // find <
    while(*m && *m!='<') {
      //putchar('1');
      //putchar(*m);
      e = m;
      m++;
    }
    if (!*m) break;
    m++;
    
    if (*m=='/') {
      // just counting, no care what
      depth--;
      if (depth) {
	//putchar('<');
	//putchar(*m);
	e = m;
      }
    } else {
      depth++;
      //putchar('<');
      e = m;
      
      // scan till end of <>
      while(*m && *m!='>') {
	// this handles <foo/>
	if (*m=='/') depth--;
	//putchar('2');
	//putchar(*m);
	e = m;
	m++;
      }
      if (!*m) break;
      if (depth) {
	//putchar(*m);
	e = m;
      }
    }

    m++;
  }

  // finished
  if (s != e) {
    printf("  >> '");
    while(s <= e && *s) {
      putchar(*s);
      s++;
    }
    printf("'\n");
    return e+1;
  }

  return NULL;
}


// NULL if t(ext) doesn't start with the p(ath) id, full prefix until q(uoting) character found (or 0 if no care)
char* strpre(char* p, char* t, char q) {
  //printf("%% p=%s\n", p);
  //printf("%% t=%s\n", t);
  //printf("%% q=%c\n", q);

  // skip any leading q
  if (q && (q=='"' || q=='\'')) {
    if (*t && *t==q) t++;
    if (*p && *p==q) p++;
  }

  // compare chars till fail
  while(*p && *t && *p!=q && *t!=q) {
    if (XTRACE) printf("...PRE p=%s t=%s\n", p, t);
    if (*p != *t) break;
    p++;
    t++;
  }
  // more letters == not match
  if (*p && *t &&
      (isalpha(*p) || isalpha(*t)))
    return NULL;

  // otherwise we're all good!
  return t;
}
  
// calls gotit on matches, only once if 'one' is true.
// returns pointer to next part of text to parse, NULL if finished
char* xpath(char* p, char* t, int one /* , callback? */) {
  if (XTRACE) printf("\nTT '%s'\n", t);
  if (XTRACE) printf("?? '%s'\n", p);
  if (!t || !p) return NULL;
  while (*p && *t) {
    switch (*p) {
    case '/': // root '/' and anyw '//'
      p++;
      if (!*p) return NULL;
      if (*p=='/') { // any depth '//'
	p++;
	while(*p && t && *t) {
	  // not care if match
	  xpath(p, t, one);
	  // TODO: if it did find a match we need to skip it!
	  // NOT CORRECT:
	  //if (!t) return NULL;

	  // if not, skip to next element
	  if (XTRACE) printf("...skipped...\n");
	  if (XTRACE) printf("TT '%s'\n", t);
	  if (*t && *t=='<') t++;
	  while(*t && *t != '<') t++;

	  // TODO: BUG: we could go negative here on depth, so...
	}
	return NULL; // fail
      } else {
	// go next direct child
	// (skip a <tag>..</tag>)
	//printf("...next child: p=%s t=%s\n", p, t);
	int depth = -1;
	xpath(p, t, one);
	do {
	  if (depth < 0) depth = 0;

	  // find start tag
	  while(*t && *t!='<') t++;
	  if (!*t) return NULL;
	  t++;
	  if (!*t) return NULL;
	  //printf("...found < depth=%d t=%s\n", depth, t);

	  if (*t == '/') {
	    depth--;
	  } else {
	    depth++;
	  }

	  // find end of tag
	  while(*t && *t!='>') {
	    if (*t == '/') depth--;
	    t++;
	  }
	  if (!*t) return NULL;
	  t++;

	  //printf("...found > depth=%d t=%s\n", depth, t);
	} while (*t && depth > 0);
	if (!*t) return NULL;
		   
	// skip till next start tag
	while(*t && *t!='<') t++;
	if (!*t) return NULL;
	
	// go back to '/', lol
	p--;

	printf("...=>next child: p=%s t=%s\n", p, t);
      }
      break;
    case '@': // attribute foo, @* any attr
      // enter start tag
      while(*t && *t!='<') t++;
      if (!*t) return NULL;

      // loop over attributes
      while(*t && *t!='>') {

	// find attribute
	while(*t && *t!='=' && *t!='>') t++;
	if (!*t || *t=='>') return NULL;
	t++;
      
	// text have '=' attribute
	char* q = *t;
	if (q=='\'' || q=='"') t++;
	if (!strpre(p, t, q)) {
	  // match!
	  // skip to content
	  while(*t && *t!='>') t++;
	  // skip id
	  char* pp = p;
	  while(*pp && isalpha(*pp)) pp++;
	  // continue
	  t = xpath(pp, t, one);
	  if (!t) return NULL;

	  // try next attr
	}
      }
      return NULL;
    case '[': return XPERR; // "has"/with 1-n, last(), last()-1, position()<3, condition [@lang='en'], [price>35],
    case '*': // any element, node()
      // skip one element in text
      while(*t && *t!='<') t++;
      if (!*t || *t!= '<') return NULL;
      while(*t && *t!='>') t++;
      if (!*t || *t!= '>') return NULL;
      p++;
      break;
    case '|': return XPERR; // or...
    case ':': return XPERR; // axis ancestor:book, following::label
    case '(': return XPERR; // text()=='foo', node()    
    default: // a-z
      if (XTRACE) printf("...tag p=%s t=%s\n", p, t);
      if (!isalpha(*p)) return NULL;
      // tag name

      // find begin tag
      while(*t && *t!='<') t++;
      if (!*t || *t!= '<') return NULL;
      t++;
      if (!*p) return NULL;
      // not right tag
      if (!strpre(p, t, 0)) return NULL;
      if (XTRACE) printf("...tag match!");
      // find end tag in text
      while(*t && *t!='>') t++;
      if (!*t || *t!= '>') return NULL;
      t++;
      // advance path
      while(*p && isalpha(*p)) p++;

      // if at end of pat then result
      if (!*p)
	return gotit(t);

      // TODO: we don't require direct following matching, just one direct child... lol

      if (XTRACE) printf("...more to go p=%s t=%s\n", p, t);
    }
  }

  return NULL;
}

void main() {
  //char* t = "<foo>FOO</foo>";
  //char* t = "<foo>FOO<bar>BAR</bar>FIE</foo>";
  //char* t = "<foo>F<fie/>OO<bar>BAR</bar>FIE</foo>";
  //char* t = "<xoo><foo>FOO<bar>BAR</bar>FIE</foo></xoo>";
  //char* t = "<xoo><abba>ABBA</abba>x sd <a>fds</a> sdf <foo>FOO<bar>BAR</bar>FIE</foo></xoo>";
  char* t = "<xoo><abba>ABBA</abba><gurk>GURK</gurk><foo>FOO<bar>BAR</bar>FIE</foo></xoo>";
  
  char* p = "foo"; // yes (must be top?) TODO: switch /foo and foo ???
  //char* p = "//bar"; // yes
  //char* p = "bar";
  //char* p = "/foo"; // don't care depth? (incorrect?)
  //char* p = "//foo"; // same
  //char* p = "xoo"; // <foo>..

  //char* p = "//fie"; // TODO: not foo
  //char* p = "//fie"; // TODO: only one match, not <bar> lol
  //char* p = "xoo//bar"; // TODO:
  //char* p = "xoo/foo"; // TODO:
  //char* p = "xoo//foo"; // yes
  
  printf("\nTT '%s'\n", t);
  printf("?? '%s'\n", p);
  char* r = xpath(p, t, 1);
  printf(".. '%s'\n", r);
}
