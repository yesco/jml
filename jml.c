// idea:
// "one heap"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#include <cstring>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#ifdef HTTPD
#define UNIX 1
#include "httpd.h"
#undef UNIX
#endif

// max number of functions
// TODO: make linked list
#ifdef CC65
#define SIZE 64
#else
#define SIZE 1024
#endif
#
// set to non-0 to trace evaluation on stderr
int trace  = 0;
// set to > 0 to write perf messages to stderr
// -1 = -q quiet mode
//  0 = normal: webserver, urls, appends
//  1 = timing
//  2 = memory allocs
//  3 = memory reallocs
int verbose  = 0;

// stats
int ramtotal = 0;
int rammax = 0;
int ramreallocs = 0;
int ramlast = 0;

// TODO: consider adding
// - https://github.com/cesanta/slre (one file)

// TODO: move this up
char* out = NULL;
char* to = NULL;
char* end = NULL;

//#define free(x) ({ unsigned _x = (unsigned) x; printf("\n%d:FREE %x\n", __LINE__, _x); /*free(x); */})

// TODO: two layers of global pointers... Hmmm. how to cleanup?
// could create local myout that takes the jmlputchar function as parameter...
typedef void (*Out)(int len, char c, char* s);

typedef int (*jmlputchartype)(int c);

//static jmlputchartype jmlputchar;

// A lambda wrapper putchar to get compatinble type
auto Lputchar = [](int c)->int { return putchar(c); };

//static auto jmlputchar = &Lputchar;
jmlputchartype jmlputchar = &putchar;


// if s != NULL append it, 
// if len == 0 actually print c - means no need capture to later process
// if len == 1 append c
void myout(int len, char c, char* s) {
  len = s ? (len >= 0 ? len : strlen(s)) : len; // TODO: if -1 what???
  // enough space?
  if (!out) {
    int sz = 1024 + ramlast;
    ramtotal += sz;
    ramlast = sz;
    if (verbose > 2) fprintf(stderr, "{--ALLOC %d--}", sz);
    to = out = (char*) malloc(sz);
    memset(out, 0, sz);
    end = out + sz;
  }

  if (to + len >= end) {
    int sz = end-out + 1024 + len;
    char* nw = (char*) realloc(out, sz);
    *to = 0;
    ramtotal += sz;
    rammax = sz > rammax ? sz : rammax;
    ramlast = sz;
    ramreallocs++;
    if (verbose > 1) fprintf(stderr, "{--REALLOC %d--}", sz);
    if (!nw) {
      fprintf(stderr, "\n%%ERRROR: realloc error!");
      exit(2);
    }
    to = to - out + nw;
    out = nw;
    end = out + sz;
  }

  if (s) {
    while (*s && len-- > 0) myout(1, *s++, NULL);
  } else if (len == 1) { 
    *to++ = c;
  } else {
    (*jmlputchar)(c);
  }
  *to = 0;
}

// safe out of web input, will html quote potential injection characters
// use this when reading from ANY external source (web, file, env variables etc)
// TODO: How effective is this?
// TODO: how does \\ handle?
void safeout(int len, char c, char* s, Out xout) {
  //myout(len, c, s); return; // TODO(jsk): wtf?
  if (len == 1) {
    if (c == '\"') xout(-1, 0, "&quot;");
    else if (c == '\'') xout(-1, 0, "&#39;");
    else if (c == '&') xout(-1, 0, "&amp;");
    else if (c == '[') xout(-1, 0, "&#91;");
    else if (c == ']') xout(-1, 0, "&#93;");
    else xout(1, c, NULL);
  } else if (len < 0) {
    char c;
    len = strlen(s);
    while ((c = *s)) safeout(1, *s++, NULL, xout);
  } else if (len == 0) {
    // nothing safe
    xout(0, c, NULL);
  }
}

// TODO: make a linked list in FLASH
typedef struct FunDef {
  char* name;
  char* args;
  char* body;
  char* time;
  char* user;
  int logpos;
} FunDef;

FunDef functions[SIZE]; // = {0};
int functions_count = 0;
int last_logpos = 0;

FILE* jml_state = NULL;
char* jml_state_name = NULL;
char* jml_state_url = NULL;

void fprintFun(FILE* f, int i) {
  FunDef *fun = &functions[i];
    
  // TODO: fill in values?
  char* comment = "";
    
  // TODO: test for faiure
  int r = 1;
  if (r > 0) r = fprintf(f, "[macro %s%s%s]", fun->name, strlen(fun->args) ? " " : "", fun->args);
  if (r > 0) {
    char* data = fun->body;
    char c;
    while ((c = *(data++))) {
      if (c == '\n') fputs("\\n", f);
      else if (c == '\r') ; // fputs("\\r", f);
      else fputc(c, f);
    }
  }
  if (r > 0) r = fprintf(f, "[/macro %d %s %s %s]\n", fun->logpos, fun->time, fun->user ? fun->user : "", comment);

  if (r <= 0) { fprintf(stderr, "\n%%Writing function %s at position %d failed\n", fun->name, i); exit(77); }
  fflush(f);

  if (verbose >= 0) fprintf(stderr, "{--APPENDED %s to file--}", fun->name);
}

void fprintAllFuns(FILE* f) {
  int i;
  for(i = 0; i < functions_count; i++)
    fprintFun(f, i);
}

// TODO: how to handle closures/RAM and GC?
void fundef(char* funname, char* args, char* body, char* crtime, int logpos, char* user) {
  //printf("\nFUNDEF: %s[%s]>>>%s<<<\n", funname, args, body);
  if (functions_count > SIZE) {
    fprintf(stderr, "\n%%OUT OF FUNCTIONS!\n");
    exit(1);
  }

  int i = functions_count;
  FunDef *f = &functions[functions_count++];

  char iso[sizeof "2011-10-08T07:07:09Z"];
  time_t now;
#ifndef CC65
  time(&now);
  strftime(iso, sizeof iso, "%FT%TZ", gmtime(&now));
#endif
  // take a copy as the data comes from transient current state of program
  f->name = strdup(funname);
  f->args = strdup(args);
  f->body = strdup(body);
  f->time = crtime ? strdup(crtime) : jml_state ? strdup(iso) : NULL;
  f->logpos = jml_state ? ++last_logpos : logpos;
  f->user = user ? strdup(user) : NULL; // TODO: hashstrings?

  if (jml_state) fprintFun(jml_state, i);
}

FunDef* findfun(char* funname) {
  int i;
  // search backwards to find the latest first
  for(i = functions_count-1; i >= 0; i--) {
    FunDef *f = &functions[i];
    if (f->name && !strcmp(funname, f->name)) return f;
  }
  return NULL;
}

typedef struct Match {
  char* start;
  char* end;
  char* endend;
} Match;

// cc65 can't handle nested functions!
#ifndef CC65  
// only knows ^ () * . $
// TODO: + [] ?
void match(char *regexp, char *text, char* fun, Out out, int subst, int global) {
#define MAXLEVELS 10
  Match m[MAXLEVELS];// = { 0 };
  memset(m, 0, sizeof(m)); // cc65
  
  // can't do forward declaration for "dynamic function"
  //int (*matchstar)(int c, char *regexp, char *text, int level);
  //int (*Xmatchhere)(char *regexp, char *text, int level);
  
  /* matchhere: search for regexp at beginning of text */
  //int Xmatchhere(char *regexp, char *text, int level) {
  auto Xmatchhere = [&] (char *regexp, char *text, int level, auto xmh, auto ms)->int {
    m[level].endend = text;
    if (!*text) return !*regexp;
    if (level >= MAXLEVELS) { fprintf(stderr, "\n%%Regexp full %s", regexp); return 0; }
    if (!*regexp) { m[0].end = text; return 1; }
    if (*regexp == '(') { m[++level].start = text; return xmh(regexp+1, text, level, xmh, ms
							      ); }
    if (*regexp == ')') { m[level].end = text; return xmh(regexp+1, text, level, xmh, ms); }
    if (regexp[1] == '*') return ms(*regexp, regexp+2, text, level, ms);
    if (*regexp == '$' && !regexp[1]) { if (!m[level].end) m[level].end = text; return *text == '\0'; }
    if (*regexp == '\\' && regexp[1] == 'n' && *text == '\n') {
      return xmh(regexp+2, text+1, level, xmh, ms);
    }
    if (*text && (*regexp == '.' || *regexp == *text)) return xmh(regexp+1, text+1, level, xmh, ms);
    return 0;
  };

  /* matchstar: search for c*regexp at beginning of text */
  //int Xmatchstar(int c, char *regexp, char *text, int level, auto ms) {
  auto Xmatchstar = [&] (int c, char *regexp, char *text, int level, auto ms)->int {
    do { /* a * matches zero or more instances */
      if (Xmatchhere(regexp, text, level, Xmatchhere, ms)) return 1;
    } while (*text && (*text++ == c || c == '.'));
    return 0;
  };

  auto Xmatch = [&] (char* regexp, char* text, int level)->int {
    if (!text || !*text) return !*regexp;
    if (*regexp == '^') return Xmatchhere(regexp+1, m[level].start = text, level, Xmatchhere, Xmatchstar);
    // TODO: not correct, as we call many times with "global" pretent there is an \n at the beginning
    if (*regexp == '\\' && regexp[1] == 'n'
        && Xmatchhere(regexp+2, m[level].start = text, level, Xmatchhere, Xmatchstar)) return 1;
    do { /* must look even if string is empty */
      if (Xmatchhere(regexp, m[level].start = text, level, Xmatchhere, Xmatchstar)) return 1;
    } while (*text++);
    return 0;
  };

  //matchstar = Xmatchstar; // backpatch for mutal recursion
  char* last = text;
  m[0] = (Match){text, text};
  while (Xmatch(regexp, text, 0) && global) {
    if (subst) {
      char* end = m[0].start;
      out(end-last, 0, last);
      last = NULL;
    } else {
      out(1, ' ', NULL);
    }
    out(1, '[', NULL);
    out(-1, 0, fun);
    int i;
    // skip the "all" first match
    for(i = 1; i < MAXLEVELS; i++) {
      // TODO: no allocat, just loop print characters?
      char* s = (m[i].start && m[i].end) ? strndup(m[i].start, m[i].end - m[i].start) : NULL;
      if (!s) break;
      out(1, ' ', NULL);
      out(-1, 0, s);
      free(s);
      if (m[i].endend) last = m[i].endend;
      if (!last && m[i].end) last = m[i].end;
      // fprintf(stderr, "   => '%s'./%s/ ... %d >%s<\n", text, regexp, i, s);
    }
    out(1, ']', NULL);
    if (last == text) break;
    text = last;
  }
  if (subst) out(-1, 0, m[0].end);
  //global = 0;
#undef MAXLEVELS    
}
#endif

// TODO: find temporary func/closures... [func ARGS]...[/func], only store in RAM and get rid of between runs...
void removefuns(char* s) {
  char *m = s, *p;
  if (!s) return;
  //fprintf(stderr, "\nREMOVEFUNS:%s<<\n", s);
  // remove [macro FUN ARGS]BODY[/macro...] and doing fundef() on them
  if (verbose > 1) fprintf(stderr, "{{removefuns:%s}}", s);
  while ((m = p = strstr(m, "[macro "))) {
    char* name = p + strlen("[macro ");
    char* spc = strchr(name, ' ');
    char* argend = strchr(name, ']');
    char* nameend = (spc < argend) ? spc : argend;
    if (nameend) *nameend = 0;
    *argend = 0;

    {

      char* args = nameend ? nameend + 1 : argend + 1;
      if (args > argend) args = "";

      {

        char* body = argend + 1;
        char* end = strstr(body, "[/macro");
        if (!end) { fprintf(stderr, "\n%%ERROR: macro not terminated: %s%s]%s\n", p,args,body); exit(3); }
        *end = 0; // terminate body

        // process end of [/macro ISO LOGPOS USER COMMENT]
        end += strlen("[/macro");

        {

          char* endend = strstr(end, "]");
          if (!endend) { fprintf(stderr, "\n%%ERROR: [/macro not terminated with ]:%s", p); exit(4); }

          {
            char* lp = strtok(end, " ]\n");
            int logpos = -1;
            if (lp) {
              logpos = atoi(lp);
              if (last_logpos == 0 && logpos == 1) {
                last_logpos = logpos;
              } else if (logpos == last_logpos+1) {
                last_logpos = logpos;
              } else if (logpos > last_logpos) {
                last_logpos = logpos;
                // TOOD: inject error message [warning log XXXX] ! (how?)
                fprintf(stderr, "\n%%[/macro.warning: logpos=%d != last_logpos=%d+1] - log missing/incomplete",
                        logpos, last_logpos);
              } else {
                // TOOD: inject error message [error log XXXX] ! (how?)
                fprintf(stderr, "\n%%[/macro.error: logpos=%d <= last_logpos=%d]", logpos, last_logpos);
              }
            }

            {

              char* tm = strtok(NULL, " ]\n");

              char* user = strtok(NULL, " ]\n");
              char* comment = user ? user + strlen(user) + 1 : NULL;

              fundef(name, args, body, tm, logpos, user);

              *end = 0;
              //fprintf(stderr, "\n[LOG >%s< >%s< >%s< >%s<]\n", lp, tm, user, comment);
                    
              memmove(m, end, endend-end+1);
              // leave the /macro function to execute to store some facts if there

              // just for cc65, it can't handle def after code
            } } } } }

  }
}

// We choose simple symetric encryption as we cannot secure the keys on
// embedded devices anyway.

// https://en.wikipedia.org/wiki/XXTEA
#define MX ((z>>5^y<<2) + (y>>3^z<<4) ^ (sum^y) + (k[p&3^e]^z))

// encrypt: btea(data, #longs (>1) keyhash (4 long))
// decrypt: btea(data, #longs (<1) keyhash (4 long))
//#include <stdint.h>
int32_t btea(int32_t* v, int32_t n, int32_t* k) {
  uint32_t z=v[n-1], y=v[0], sum=0, e, DELTA=0x9e3779b9;
  int32_t p, q ;
  if (n > 1) {          /* Coding Part */
    q = 6 + 52/n;
    while (q-- > 0) {
      sum += DELTA;
      e = (sum >> 2) & 3;
      for (p=0; p<n-1; p++) y = v[p+1], z = v[p] += MX;
      y = v[0];
      z = v[n-1] += MX;
    }
    return 0 ;
  } else if (n < 1) {  /* Decoding Part */
    n = -n;
    q = 6 + 52/n;
    sum = q*DELTA ;
    while (sum != 0) {
      e = (sum >> 2) & 3;
      for (p=n-1; p>0; p--) z = v[p-1], y = v[p] -= MX;
      z = v[n-1];
      y = v[0] -= MX;
      sum -= DELTA;
    }
    return 0;
  }
  return 1;
}

// gcc only... :-(
//#define HEX2INT(x) ({ int _x = toupper(x); _x >= 'A' ? _x - 'A' + 10 : _x - '0'; })
char HEX2INT(int x) {
  int _x = toupper(x);
  return  _x >= 'A' ? _x - 'A' + 10 : _x - '0';
}

// used to be nested in macro_subst
// but for cc65 we move out, it's not
// called recursivley so we define vars here
// TODO: make cleaner (pass struct?)

#define MAX_ARGS 10
int argc = 0;
char* farga[MAX_ARGS] = {0};
int fargalen[MAX_ARGS] = {0};
char* arga[MAX_ARGS] = {0};

void parseArgs(char* fargs, char* args) {
  int i = 0;

  argc = 0;
  // neeed?
  farga[0] = 0;
  fargalen[0] = 0;
  arga[0] = 0;
  
  // fill in farga[] and fargalen[] from fargs
  while (*fargs) {
    int len = 0;
    while (*fargs == ' ') fargs++;
    if (!*fargs) break;
    farga[argc] = fargs;
    while (*fargs && *fargs != ' ') { fargs++; len++; }
    fargalen[argc] = len;
    argc++;
    if (argc > MAX_ARGS) { fprintf(stderr, "\n%%parseArgs MAX_ARGS too small!\n"); exit(4); }
  }
  // match extract each farg with actual arg
  // skip space
  while (*args == ' ') *args++ = 0;
  while (*args && argc && i < argc + (farga[argc-1][0] == '$')) {
    char last = 0;

    // skip space
    while (*args == ' ') *args++ = 0;

    arga[i] = args;

    // read arg (till next unquoted space, or end)
    while (*args && (last == '\\' || *args != ' ')) last = *args++;
    i++;
  }
}
#undef MAX_ARGS

void macro_subst(Out out, FunDef* f, char* args) {
  char* body = f->body;
  char* fargs = f->args;
  char c;

  parseArgs(fargs, args);
        
  while ((c = *body)) {
    if (c == '$' || c == '@' ) {
      // find named prefix if match formal argument name
      int i = 0;
      while (i < argc && strncmp(body, farga[i], fargalen[i])) i++;
            
      // substitute
      if (i < argc) {
        out(-1, 0, arga[i]);
        body += fargalen[i];
      } else {
        // TOOD: this seems to loop forever at problem...
        fprintf(stderr, "\n%%Argument: can't find argument in '%s'\n", strtok(body, " "));
        return;
      }
    } else {
      if (c == '\\') out(1, *body++, NULL);
      out(1, *body++, NULL);
    }
  }
  return;
} 

#define CONTENT_TYPE_MULTI "Content-Type: multipart/form-data"
#define COOKIE "Cookie:"

char* jmlbody_store = NULL;
char* jmlheader_cookie = NULL;
char* jml_user = NULL;

// these used to be inline in funsubst
// "only" GCC handles inline functions

// do we even need this one?
// TODO: maybe just use a global?
Out runout;

void skipspace(char **pos) {
  while(**pos == ' ') (void) (*pos)++;
}
    
void outspace() {
  runout(1, ' ', NULL);
}

// if FUN => '[FUN VAL]'
void outfun(char* fun, char* val) {
  if (!val || !*val) return;
  if (fun) {
    runout(1, '[', NULL);
    runout(-1, 0, fun);
    outspace();
  }
  runout(-1, 0, val);
  if (fun) runout(1, ']', NULL);
  outspace();
}
    
void outnum(int n) {
  //int len = snprintf(NULL, 0, "%d", n);
  //char x[len+1]; // gcc only, lol
  char x[20]; // enought? :-)
  int len = snprintf(x, 20, "%d", n);
  runout(len, 0, x);
}

// make it global for cc65 (no local func!)
char* rest;
char* nextargs;

char* next() {
  char* a = nextargs;
  char* r = strtok(a, " ");
  nextargs = NULL; // strtok will get only once
  rest += r ? strlen(r) + 1 : 0;
  return r ? r : (char*) ""; // returning NULL may crash stuff...
}
int num() { return atoi(next()); }

// At the current position in outstream get the body of funname
// and replace the actual arguments into positions of formals.
void funsubst(Out out, char* funname, char* args) {
  // either call out(...) and return
  // or fall through and
  // if s != NULL then this is output value
  // otherwise it stringifies the numeric value r
  // s = arg to make an ID function, next() to get next argument (modifies arg)

  int r = -4711;
  char* s = NULL;
  FunDef *f = findfun(funname);

  rest = args;
  nextargs = args;
    
  // TOOO: check for overwrite?
  runout = out;

  //printf("%% funcall=%s args=%s >%s<\n", funname, args, jmlheader_cookie);
  if (f) { macro_subst(out, f, args); return; }
  else if (!strcmp(funname, "inc")) r = num() + 1;
  else if (!strcmp(funname, "dec")) r = num() - 1;
  else if (!strcmp(funname, "-")) r = num() - num();
  else if (!strcmp(funname, "/")) r = num() / num();
  else if (!strcmp(funname, "%")) r = num() % num();
  else if (!strcmp(funname, ">")) r = num() > num();
  else if (!strcmp(funname, "<")) r = num() < num();
  else if (!strcmp(funname, ">=")) r = num() >= num();
  else if (!strcmp(funname, "<=")) r = num() <= num();
  else if (!strcmp(funname, "=")) r = num() == num();
  else if (!strcmp(funname, "!=")) r = num() != num();
  else if (!strcmp(funname, "+")) {
    char* x;
    r = 0; 
    while (*((x = next()))) r += atoi(x);
  } else if (!strcmp(funname, "*")) {
    char* x;
    r = 1;
    while (*((x = next()))) r *= atoi(x);
  } else if (!strcmp(funname, "iota")) {
    int f = 0, t = num(), s = 1;
    char* x = next();
    if (*x) {
      f = t;
      t = atoi(x) + 1;
      x = next();
    }
    if (*x) s = atoi(x);
    while (f < t) {
      outnum(f);
      f += s;
      outspace();
    }
    return;
  }
  else if (!strcmp(funname, "if")) {
    int e = num(); char *thn = next(), *els = next();
    thn = thn ? thn : (char*) "";
    els = els ? els : (char*) "";
    s = e ? thn : els;
  }
  else if (!strcmp(funname, "empty")) { char* x = next(); r = !x ? 1 : !*x; }
  else if (!strcmp(funname, "has")) { char* x = next(); r = !!x && *x > 0; }
  else if (!strcmp(funname, "is")) { char* x = next(); r = !!x && *x && (x[0] != '0' || x[1]) > 0; }
  else if (!strcmp(funname, "number")) { char* x = next(); r = *x; while(*x && r) r = isdigit(*x++) && r && !*next(); }
  else if (!strcmp(funname, "alpha")) { r = 0; while(*args) r += isalpha(*args++) > 0; }
  else if (!strcmp(funname, "equal")) r = strcmp(next(), next()) == 0;
  else if (!strcmp(funname, "cmp")) r = strcmp(next(), next());
  // we can modify the input strings, as long as they get shorter...
  else if (!strcmp(funname, "lower")) { char* x = args; s = args; while ((*x = tolower(*x))) x++; }
  else if (!strcmp(funname, "upper")) { char* x = args; s = args; while ((*x = toupper(*x))) x++; }
  else if (!strcmp(funname, "length")) { r = 0; while (*next()) r++; }
  else if (!strcmp(funname, "bytes")) { r = 0; while (*args++) r++; }
  else if (!strcmp(funname, "nth")) { int n = num(); s = ""; while (n-- > 0) s = next(); }
  // TODO: parse json - https://github.com/zserge/jsmn/blob/master/jsmn.c
  else if (!strcmp(funname, "map")) {
    char* fun = next(), *x;
    while (*(x = next())) outfun(fun, x);
    return;
  } else if (!strncmp(funname, "filter", 6)) {
    char* pred = next();
    char* fun = !strcmp(funname, "filter-do") ? next() : (char*) "identity";
    char* x;
    while (*(x = next())) {
      // [[if [pred X] fun ignore] X]
      out(1, '[', NULL);
      out(1, '[', NULL);
      out(-1, 0, "if ");
      outfun(pred, x);
      out(-1, 0, fun);
      out(-1, 0, " ignore");
      out(1, ']', NULL);
      outspace();
      out(-1, 0, x);
      out(1, ']', NULL);
      outspace();
    }
    return;
  } else if (!strcmp(funname, "after")) {
    // TODO search for '\$' doesn't work... and '$' gives macro usage error...
    char* x = args;
    char* find = next();
    x += strlen(find) + 1;
    {
      char* f = strstr(x, find);
      if (f) out(-1, 0, f + strlen(find));
      return;
    }
  } else if (!strcmp(funname, "before")) {
    char* x = args;
    char* find = next();
    x += strlen(find) + 1;
    {
      char* f = strstr(x, find);
      if (!f) return;
      *f = 0;
      out(-1, 0, x);
      return;
    }
  } else if (!strcmp(funname, "prefix")) {
    char* a = next();
    char* b = next();
    while (*a && *b) {
      char *xa = a, *xb = b;
      char* p = (char*) a; // TODO: remove all const!
      while (*xa && *xb && *xa++ == *xb++) p++;
      *p = 0; // cut at end of common prefix
      b = next();
    }
    out(-1, 0, a);
    return;
#ifndef CC65
  } else if (!strcmp(funname, "match-do")) {
    // TODO: name it match-do?
    // TODO: - [match F a(b*)(cd*)e(.*)f acexxxxxfff] => [F c xxxxx] ... b position missing, quote mode?
    // TODO: => [F {} {c} {xxxxx}] ???
    char* fun = next();
    char* regexp = next();
    skipspace(&rest);
    match(regexp, rest, fun, out, 0, 1);
    return;
  } else if (!strcmp(funname, "subst-do")) {
    // Like match-do but replaces matches
    char* fun = next();
    char* regexp = next();
    if (verbose > 1) fprintf(stderr, "\n%%subst-do fun=%s< regexp=%s<\n", fun, regexp);
    skipspace(&rest);
    match(regexp, rest, fun, out, 1, 1);
    return;
#endif
  } else if (!strcmp(funname, "concat")) {
    char* d = args; s = args; 
    // essentially it concats strings by removing spaces
    // and copying the rest of string over the space
    while (*args) {
      // read arg (till next unquoted space, or end)
      char last = 0;
      while (*args && (last == '\\' || *args != ' ')) *d++ = last = *args++;
      skipspace(&args);
    }
    *d = 0;
  } else if (!strcmp(funname, "join")) {
    char* delim = next();
    char* val = next();
    while (val && *val) {
      out(-1, 0, val);
      val = next();
      if (!val || !*val) break;
      out(-1, 0, delim);
    }
    out(-1, 0, val);
    return;
  } else if (!strncmp(funname, "split", 5)) {
    char* x = args;
    char* fun = !strcmp(funname, "split-do") ? next() : NULL;
    char* needle = next();
    if (fun) x += strlen(fun) + 1;
    x += strlen(needle) + 1;
    ///        printf("SPLIT: >%s< >%s< DATA>%s<\n", fun, needle, x);
    while (*x) {
      char* f = strstr(x, needle);
      if (!f) break;
      *f = 0;
      outfun(fun, x);
      x = f + strlen(needle);
    }
    // output rest
    outfun(fun, x);
    return;
  } else if (!strcmp(funname, "substr")) {
    int start = num();
    int len = num();
    int l = strlen(rest);
    if (start > l) return;
    if (start < 0) if (len < 0) len = -len;
    if (start < 0) start = l + start;
    if (start < 0) start = 0;
    s = rest + start;
    if (len < 0) len = l + len;
    if (len > l) len = l - start;
    if (len < 0) len = 0;
    fprintf(stderr, "\n%% substr len=%d", len);
    *(s+len) = 0;
    rest = NULL;
  } else if (!strcmp(funname, "uuid")) {
    // TODO: consider using UUID/GUID random generator for unique ID
    // - https://github.com/marvinroger/ESP8266TrueRandom
    // This is modified from - http://stackoverflow.com/questions/7399069/how-to-generate-a-guid-in-c
    int t = 0;
    char *tmpl = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    char *hex = "0123456789ABCDEF-";
    char* p = tmpl;

    srand(clock());
    while (*p) {
      int r = rand () % 16;
      switch (*p) {
      case 'x' : out(1, hex[r], NULL); break;
      case 'y' : out(1, hex[r & 0x03 | 0x08], NULL); break;
      default  : out(1, *p, NULL); break;
      }
      p++;
    }
    return;
  } else if (!strcmp(funname, "decode")) {
    // TODO: add matching encode?
    char* x = s = args;
    while (*x) {
      if (*x == '+') *x = ' ';
      if (*x == '%') {
        *x = HEX2INT(*(x+1)) * 16 + HEX2INT(*(x+2));
        memmove(x+1, x+3, strlen(x+3) + 1);
      }
      // TODO: unsafe so we decode to [] allowing any code...
      // http://localhost:1111/+/%5b*%209%209%5d
      if (*x == '[') *x = '{';
      if (*x == ']') *x = '}';
      x++;
    }
  } else if (!strncmp(funname, "encrypt", 7)) {
    char hex[] = "0123456789ABCDEF";
    int32_t k[4]; // long assumed to be 4 bytes!

    // find key to use
    // TODO: look it up from macro
    char* key = strstr(funname, "/");
    int kl;
    char* last;
    int l;
    int n;
    int32_t* data;
    int i;
    char* d;

    if (key)
      key = key + 1;
    else
      key = "1234123412341234";

    memset(k, 0, sizeof(k));
    kl = strlen(key);
    memcpy(k, key, kl > 16 ? 16 : kl);

    // trim space before and at end
    skipspace(&args);
    last = args + strlen(args) - 1;
    while (*last == ' ') *last-- = 0;
    l = strlen(args);

    // encrypt-eval function, has an interesting action:
    // it'll replace all {} to [] before encrypting.
    // This means, when it's decrypted with decrypt-eval
    // it would automatically execute any code...
    if (!strcmp(funname, "encrypt-eval")) {
      char* e = args;
      while (*e) {
        if (*e == '{') *e = '[';
        if (*e == '}') *e = ']';
        e++;
      }
    }
            
    n = (l+3)/4;
    // we need to pad to at least 8 bytes, for n == 1 no encryption!
    if (n < 2) n = 2;
    // TODO: can we do it in place? with no allocation? I guess we may need 4 bytes extra...
    // we could just move it back over the [encrypt but we'd need to align it on 4byte boundary
    //long data[n]; // cc65 can't
    // again: assumption long == 4 bytes, hmmm
    data = (int32_t*) malloc(n*sizeof(*data));
    memset(data, 0, n*sizeof(*data));
    memcpy(data, args, l+1); // incl \0
    btea(data, n, k);

    d = (char*)data;
    out(1, '{', NULL);
    for(i = 0; i < n*4; i++) {
      unsigned char c = *d++;
      out(1, hex[c / 16], NULL);
      out(1, hex[c % 16], NULL);
    }
    free(data);
    out(1, '}', NULL);
    return;
  } else if (!strncmp(funname, "decrypt", 7)) {
    // find key to use
    // TODO: look it up from macro
    char* key = strstr(funname, "/");
    int32_t k[4];
    int kl;
    char* last;
    char* d;
    char* p;
    char a;
    char b;
    int n;
    int32_t* data;
    int eval;
    int i;

    if (key)
      key = key + 1;
    else
      key = "1234123412341234";

    memset(k, 0, 16);
    kl = strlen(key);
    memcpy(k, key, kl > 16 ? 16 : kl);

    // trim leading space (and '{')
    while (*args && (*args == ' ' || *args == '{')) args++;

    // trim space at end
    last = args + strlen(args) - 1;
    while (*last == ' ') *last-- = 0;
        
    // decode hex inplace
    d = args;
    p = args;
    while (*p && *p != '}') {
      a = *p++;
      b = *p++;
      *d++ = HEX2INT(a) * 16 + HEX2INT(b);
    }
    *d++ = 0;
        
    // TODO: refactor, same as above?
    n = (d - args - 1)/4;
    data = (int32_t*) malloc(n*sizeof(*data));
    memset(data, 0, n*sizeof(*data));
    memcpy(data, args, n*sizeof(*data));
    btea(data, -n, k);

    eval = !strcmp(funname, "decrypt-eval");
    // copy it out
    d = (char*)data;
    for(i = 0; i < n*4; i++) {
      if (!*d) break;
      if (!eval) {
        if (*d == '[') *d = '{';
        if (*d == ']') *d = '}';
      }
      out(1, *d++, NULL);
    }
    free(data);
    return;
  } else if (!strcmp(funname, "data")) {
    char* id;
    char* data;
    char* name;

    s = args;
    id = next();
    if (!*id) return;
    data = rest;
    // prefix the name with 'data-'
    name = (char*) malloc(strlen(id)+1+5);
    name[0] = 0;
    strcat(name, "data-");
    strcat(name, id);
    fundef(name, "", data,
           /*crtime*/NULL, // TODO: fill in
           /*logpos*/0, // TODO: fill in
           jml_user);
    s = data;
    free(name);
  } else if (!strncmp(funname, "eval/", 5)) { // safe eval!
    char* pattern = funname + 5;
    char* ret = rest;
    int parens = 0;
    rest = strchr(rest, '{');
    while (rest && *rest == '{') {
      // extract fun
      char* fun = rest + 1;
      char next;
      char* endfun = fun;
      char* ok;
      char last;

      parens++;
      while ((next = *endfun)) {
        if (*endfun == '}' || *endfun == ' ') { *endfun = 0; break; }
        endfun++;
      }

      // check if fun in pattern/allowed funcs
      ok = strstr(pattern, fun);
      if (!ok) return;
      last = *(ok+strlen(fun));
      if (*(ok-1) != '/' || (last != '/' && last)) return;
            
      // restore
      *endfun = next;

      // ok
      *rest = '[';

      // convert any '}' to ']' until '['
      while (*rest++ && *rest != '{') {
        if (*rest == '}') {
          if (parens-- <= 0) return;
          *rest = ']';
        }
      }
    }
    if (parens) return;
    s = ret;
#ifndef CC65
  } else if (!strcmp(funname, "wget")) { // TODO: "remove", replace by guaranteed message...
    // TODO: make a wget/FUNC/FUNC1/FUNC2 that will allow FUNC calls from source with name FUNCN...
    // TODO: bad that it's synchronious
    //int result(void* data, char* s) {
    auto result = [&] (void* data, char* s)->int {
      if (!s) return 1;
      // quote [] so no (unsafe) evaluation...
      char *p = s, c;
      while((c = *p++)) {
        if (c == '[') out(1, '{', NULL);
        else if (c == ']') out(1, '}', NULL);
        else out(1, c, NULL);
      }
      return 1; // get more...
    };
	
    //TODO:
    //wget(NULL, next(), result);
    out(-1, 0, "%(wget disfunctional )%");
    return;
#endif
  } else if (!strcmp(funname, "message")) {
    // echo "[wget [route-data [content-hash Hello! How are you?]]/id]" | ./run -q -t
    // TODO: this will add an entry to storage and then try to send
    //
    //    https://en.wikipedia.org/wiki/AllJoyn
    //    https://en.wikipedia.org/wiki/Internet_Storage_Name_Service
    //    https://en.wikipedia.org/wiki/Service_Location_Protocol
    //    https://en.wikipedia.org/wiki/XMPP
    char* dest = next();
    char* id;
    char* data;
    char* name;

    // char* expiry = num();
    s = args;
    id = next();
    if (!*id) return;
    data = rest;
    name = (char*) malloc(strlen(id)+1+8);
    name[0] = 0;
    strcat(name, "message-");
    strcat(name, id);
    fundef(name, "", data, NULL, 0, jml_user);
    // TOOD: send message, lol
    s = data;
    free(name);
  } else if (!strcmp(funname, "cookie")) {
    // TODO: how to make functional? infuse at "webcall resolution level"
    char* want = next();
    char* p = jmlheader_cookie;
    char* name;

    if (!want || !p) return;
    name = strchr(p, ' ');
    if (!name) name = strchr(p, '=');
    while (name) {
      char* val = strtok(NULL, " ;"); // space is converted to %20 I think
      if (!val) break;
      // printf("\n%% want>%s< name>%s< val>%s<\n", want, name, val);
      if (strcmp(want, name) == 0) out(-1, 0, val);
      name = strtok(NULL, " =");
    }
    return;
  } else if (!strcmp(funname, "funcs")) {
    char* prefix = next(); // optional
    unsigned char* bigfix = (unsigned char*) next(); // optional
    int i;
    for(i = 0; i < functions_count; i++) {
      char* name = functions[i].name;
      if (strcmp(name, prefix) < 0) continue;
      if (*bigfix ? strcmp(name, (const char*)bigfix) > 0
          : strncmp(name, prefix, strlen(prefix))) continue;
      out(-1, 0, name);
      out(1, ' ', NULL);
    }
    return;
  } else if (!strcmp(funname, "fargs")) {
    FunDef* f = findfun(next());
    char* p;
    if (!f) return;
    p = f->args;
    while (*p) {
      // dequote [ and ]?
      if (*p == '$' || *p == '@') out(1, '\\', NULL);
      out(1, *p++, NULL);
    }
    return;
  } else if (!strcmp(funname, "fbody")) {
    FunDef* f = findfun(next());
    char *b;
    char c;

    if (!f) return;
    // quote it
    b = f->body;
    while (*b) {
      if (*b == '[') out(1, '{', NULL);
      else if (*b == ']') out(1, '}', NULL);
      else out(1, *b, NULL);
      b++;
    }
    return;
  } else if (!strcmp(funname, "ftime")) {
    FunDef* f = findfun(next());
    if (!f || !f->time) return;
    out(-1, 0, f->time);
    return;        
#ifndef CC65
  } else if (!strcmp(funname, "time")) {
    time_t now;
    time(&now);
    char iso[sizeof "2011-10-08T07:07:09Z"];
    strftime(iso, sizeof iso, "%FT%TZ", gmtime(&now));
    out(-1, 0, iso);
    return;
    // TODO: add parsing to int with strptime or gettime
#endif
  } else if (!strcmp(funname, "FAIL")) {
    // default, can be overridden
    return;
  } else {
    fprintf(stderr, "\n%%(%s %s)%%", funname, args);
    out(-1, 0, "[FAIL ");
    out(-1, 0, funname);
    out(1, ' ', NULL);
    out(-1, 0, args);
    out(1, ']', NULL);
    return;
  }

  if (s) {
    out(-1, 0, s);
  } else {
    outnum(r);
  }
}

int run(char* start, Out out) {
  char* s = start;
  int substcount = 0;
  char* exp = NULL;
  char* funend = NULL;
  char c;
  int doprint = 1;
  int quote = 0;

  runout = out; // do we even need to pass it around?
  if (!start) return 0;
  removefuns(s);

  while ((c = *s)) {
    if (c == '\\') quote = 1;
    if (!quote && c == '[') { // "EVAL"
      // once we find an expression, we no longer can print/remove
      doprint = 0;
      if (exp) {
        exp--;
        // copy since '['
        while (exp < s) 
          out(1, *exp++, NULL);
      }
      exp = s + 1;
      funend = NULL;
    } else if (exp) {
      // innermost function call, replace by body etc...
      if (!quote && c == ']') { // "APPLY"
        char* p = exp;
        char* args = NULL;
        char* funname;

        if (!funend) {
          funend = s;
          args = "";
        } else {
          args = funend + 1;
        }

        *funend = 0;
        funname = p;
        *s = 0;
        funsubst(out, funname, args);
        substcount++;

        exp = NULL;
        funend = NULL;
      } else if (!funend && c == ' '){
        funend = s;
      } else {
        // expression skip till ']'
      }
    } else if (doprint) {
      out(0, c, NULL);
    } else { // retain for next loop
      out(1, c, NULL);
    }

    if (c != '\\') quote = 0;
    // next
    s++;
  }
  // end
  //*to = 0;
  return substcount;
}

// NOT! assumes one mallocated string in, it will be freed
// no return, just output on "stdout"
// TODO: how to handle macro def/invocations over several lines?
int oneline(char* s, jmlputchartype putt) {
  //auto oneline = [] (char* s, auto putt)->int {
  clock_t start = clock();
  // temporary change output method
  //jmlputchartype stored = jmlputchar;
  auto stored = jmlputchar;
  int reductions = 0;
  int cycles = 0;
  int r;
  int t;

  //fprintf(stderr, "\n!!!!!!!!!!!!!!!!!!!%s<<<<<<<\n", s);
  if (!s || !*s) return 0;
  jmlputchar = putt;

  do {
    // print each stage
    if (trace) {
      char *p = s;
      fprintf(stderr, ">>>");
      while (*p) fputc(*p++, stderr);
      fprintf(stderr, "<<<\n");
    }

    r = run(s, myout);
    cycles++;
    if (!r) break;
    reductions += r;
        
    // TODO: maybe make run return out? simplify...
    ramlast = strlen(s);
    free(s);
    s = out;
    end = to = out = NULL;
  } while (s && *s);
  //printf("%s", out); fflush(stdout);

  // free(s);
  free(out);
  end = to = out = NULL;
    
  // restore output method
  jmlputchar = stored;

  // stats
  t = (clock() - start) * (1000000 / CLOCKS_PER_SEC); // noop!
  if (verbose > 0) fprintf(stderr, "\n{--%d reductions for %d cycles in %d us--}", reductions, cycles, t);
  if (verbose > 0) fprintf(stderr, "{--maxlen: %d bytes reallocs %d--}\n", rammax, ramreallocs);

  ramtotal = 0;
  rammax = 0;
  ramreallocs = 0;
  ramlast = 0;

  return t;
};

#ifndef CC65
char* freadline(FILE* f) {
  size_t len = 0;
  char* ln = NULL;
  int n = getline(&ln, &len, f);

  if (n < 0 && ln) { free(ln); ln = NULL; }
  return ln;
}
#endif

#ifdef HTTPD
static void jmlheader(char* buff, char* method, char* path) {
  // Init on new connection
  if (!buff && !method && !path) {
    if (jmlheader_cookie) {
      free(jmlheader_cookie);
      jmlheader_cookie = NULL;
    }
    if (jmlbody_store) {
      free(jmlbody_store);
      jmlbody_store = NULL;
    }
    if (jml_user) {
      free(jml_user);
      jml_user = NULL;
    }
  }

  // TODO: extract encoding type andn handle multipart/form-data (for file upload...)
  if (buff && strncmp(buff, CONTENT_TYPE_MULTI, strlen(CONTENT_TYPE_MULTI)) == 0) {
    fprintf(stderr, "\n%% HEADER.cannot_parse.TODO: %s\n", buff);
  } else if (buff && strncmp(buff, COOKIE, strlen(COOKIE)) == 0) { 
    char* user;
    // https://tools.ietf.org/html/rfc6265
    jmlheader_cookie = strdup(buff + strlen(COOKIE));
    user = strstr(jmlheader_cookie, "user=");
    if (user) {
      char* end;
      user += 5;
      end = strchr(user, ' ');
      if (!end) strchr(user, ';');
      if (!end) end = user + strlen(user);
      jml_user = strndup(user, end-user);
    }
    fprintf(stderr, "\n%% HEADER.cookie: %s\n", buff);
  } else {
    // fprintf(stderr, "\n%% HEADER.ignored: %s\n", buff);
  }
}

static void jmlbody(char* buff, char* method, char* path) {
  if (buff && strcmp(method, "POST") == 0) {
    //printf("BODY: %s\n", buff);
    if (jmlbody_store) {
      free(jmlbody_store);
      jmlbody_store = NULL;
    }
    jmlbody_store = strdup(buff);
  } else {
    //printf("BODY.ignored: %s\n", buff);
  }
}

int doexit = 0;

int _req= -1;

int putt(int c) {
  //auto putt = [&](int c)->int {
  static int escape = 0;
  char ch = c;
  
  if (escape && c == 'n') c = '\n'; //{ write(req, "<br>", 4); c = '\n'; }
  if (escape && c == 't') c = '\t';
  if ((escape = (c == '\\'))) return 0;

  return write(_req, &ch, 1);
}

// assume path is writable
static void jmlresponse(int req, char* method, char* path) {
  _req= req; // for putt

  char* args;
  if (verbose >= 0) fprintf(stderr, "\n{------------------------------ '%s' '%s' --}", method, path);
  if (verbose > 1) fputc('\n', stderr);
  if (!strcmp("/exit", method)) {
    doexit = 1;
    return;
  }

  // args will point to data after '?' or ' '
  args = strchr(path, '?');
    
  if (strcmp(method, "POST") == 0 && jmlbody_store) args = jmlbody_store;

  if (!args) args = strchr(path, ' ');
  if (!args) args = strchr(path+1, '/');
  if (!args) {
    args = "";
  } else {
    *args++ = 0;
  }

  // skip '/' if '/foo' not found
  // TODO: for security - disallow!!!
  if (*path == '/' && !findfun(path)) path++;
    
  // TODO: errhhh..
  if (out) free(out); end = to = out = NULL;

  myout(-1, 0, "HTTP/1.1 200 OK\r\n");
  myout(-1, 0, "Content-Type: text/html\r\n");
  myout(-1, 0, "\r\n");

  // build structured string to do eval on
  // 1. /fun?foo=42&bar=fish    => [/fun foo=42&bar=fish]
  // 2. /fun?foo+bar            => [/fun foo bar]
  // 2. /fun%20foo%20bar        => [/fun foo bar]
  // TODO: 4. /?[fun+foo+bar]   => [/fun foo bar]
  // if '/fun' doesn't exists it'll try 'fun' -- TODO: safety problem? ;-)
  myout(1, '[', NULL);
  safeout(-1, 0, path, myout);
  myout(1, ' ', NULL);

  // TODO: unsafe to allow call of any function, maybe only allow call "/func" ?
  if (strchr(args, '=')) { // url on form /fun?var1=value1&var2=value2
    // We wrap all extractions in a big decode, half non-kosher
    myout(-1, 0, "[decode ");

    // TODO: could extract parameters in correct order.
    // for now, just extract values and put spaces between
    // if in correct order, then it'll "work". LOL!
    char* name = strtok(args, "=");
    while (name) {
      char* val = strtok(NULL, "&");
      // fprintf(stderr, "\n==ARG>%s< = >%s<\n", name, val);
      if (strcmp(name, "submit") != 0) { // not submit
        myout(1, ' ', NULL);
        safeout(-1, 0, val, myout);
      }

      name = strtok(NULL, "=");
    }

    myout(1, ']', NULL);
  } else { // url stuff on form /fun?arg1+arg2
    char c;
    myout(-1, 0, "[decode ");
    // replace a/b/c => a b c
    while ((c = *args++)) {
      if (c == '/') safeout(1, ' ', NULL, myout);
      else safeout(1, c, NULL, myout);
    }
    // safeout(-1, 0, args);
    myout(1, ']', NULL);
  }

  myout(1, ']', NULL);
    
  //fprintf(stderr, "OUT>>>%s<<<\n", out);

  // Cleanup state
  // TODO: make this more safe by move to init routine?
  if (jmlbody_store) {
    free(jmlbody_store);
    jmlbody_store = NULL;
  }
  // Not cleaning out jmlheader_cookie

  // make a copy
  {
    char* line = strdup(out);
    //fprintf(stderr, "\n\n++++++OUT=%s\n", out);
    //fprintf(stderr, "\n\n+++++LINE=%s\n", line);
    if (out) free(out); end = to = out = NULL;

    oneline(line, putt);
    // Note: line is freed by oneline, output putt
  }
}
#endif

auto putstdout = [](int c)->int {
  static int escape = 0;
  if (escape && c == 'n') c = '\n';
  if (escape && c == 't') c = '\t';
  if ((escape = (c == '\\'))) return 0;
  return fputc(c, stdout);
};

#ifndef CC65
int loadfile(FILE* f) {
  last_logpos = 0; // this is "defined" per file
  verbose--; // make it somewhat more silent during startup
  rewind(f);
  while (f && !feof(f)) {
    char* line = freadline(f);
    int len;
    if (!line) break;
    len = strlen(line);
    if (len) {
      // remove end newline
      if (line[len-1] == '\n') line[len-1] = 0;
      if (*line) oneline(line, putstdout);
    }
  }
  if (out) { free(out); end = to = out = NULL; }
  verbose++;
  return 0;
}
#endif

int main(int argc, char* argv[]) {
  // parse arguments
  int argi = 1;
  int web = 0;

  jml_state_name = "jml.state";

#ifndef CC65
  FILE* f = fopen(jml_state_name, "a+"); // position for read beginning, write always only appends
#endif

  memset(functions, 0, sizeof(functions));
  //    jmlputchar = putchar;

#ifndef CC65
  if (f) loadfile(f);
#endif

  while (argc > argi) {
    if (!strcmp(argv[argi], "-h")) {
      printf("./run [-q(uiet)] [-t(race)] [-w(eb) [PORT]] [-v(erbose)] [-h(elp)] [file.state [...]]\n");
      exit(1);
    } else if (!strcmp(argv[argi], "-v")) {
      argi++;
      verbose++;
    } else if (!strcmp(argv[argi], "-q")) { // quiet
      argi++;
      verbose = -1;
    } else if (!strcmp(argv[argi], "-t")) {
      argi++;
      trace = 1;
    } else if (!strcmp(argv[argi], "-w")) {
      argi++;
      web = argc > argi ? atoi(argv[argi]) : 0;
      if (web > 0)
        argi++;
      else
        web = 1111;
    } else if (!strcmp(argv[argi], "-s")) {
      argi++;
      web = -1;
    } else if (argv[argi][0] != '-') {
      // Read previous state(s)
      jml_state_name = argv[argi];
      argi++;
#ifndef CC65
      if (f) fclose(f);
      if (verbose > 0) fprintf(stderr, "\n%% loading file: %s\n", jml_state_name);
      f = fopen(jml_state_name, "a+"); // position for read beginning, write always only appends
      if (loadfile(f)) fprintf(stderr,
                               "\n%%%% Error loading file: %s (run with -v -v -v ... to get more verbose)\n", jml_state_name);
#endif
    }
  }
    
  // keep last file open for append
  //#ifndef CC65 // TODO: wrong?
  jml_state = f;
  //#endif
  //fprintf(stderr, "trace=%d web=%d state=%s\n", trace, web, state);
    
  // Start the web server
#ifndef CC65
  if (web) {
#ifdef HTTPD
    int www = httpd_init(web);
    if (www < 0 ) { fprintf(stderr, "ERROR.errno=%d\n", errno); return -1; }
    if (verbose >= 0)
      fprintf(stderr, "\n{{--%s: Webserver started on port=%d--}}\n", jml_state_name, web);
    doexit = 0;
    // simple single threaded semantics/monitor/object/actor
    while (!doexit) {
      httpd_next(www, jmlheader, jmlbody, jmlresponse);
    }
#endif
  }
  else
#endif
    {
      char* line = (char*) malloc(10);
      int linen = 10;
      do {

        // TODO: make it part of non-blocking loop!
        if (verbose >= 0) fprintf(stderr, "\njml> ");
#ifdef CC65
        {
          // function unknown on CC65... write it?

          int n = getline(&line, &linen, stdin);
        }
#endif	    
#ifndef CC65
        line = freadline(stdin);
#endif	    
        if (line) {
          int len = strlen(line);
          // remove end newline
          if (line[len-1] == '\n') line[len-1] = 0;
          // TODO: how to handle macro def/invocations over several lines?
          oneline(line, putstdout);
          fflush(stdout);
        }
      } while (line);
    }

#ifndef CC65
  if (jml_state) fclose(jml_state);
#endif
} 
