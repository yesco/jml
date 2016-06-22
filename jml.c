// idea:
// "one heap"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>

#define UNIX 1
#include "httpd.h"
#undef UNIX

// max number of functions
// TODO: make linked list
#define SIZE 1024

// TODO: consider adding
// - https://github.com/cesanta/slre (one file)

// TODO: move this up
char* out = NULL;
char* to = NULL;
char* end = NULL;

// consider using UUID/GUID random generator for unique ID
// - https://github.com/marvinroger/ESP8266TrueRandom

//#define free(x) ({ unsigned _x = (unsigned) x; printf("\n%d:FREE %x\n", __LINE__, _x); /*free(x); */})

// TODO: two layers of global pointers... Hmmm. how to cleanup?
// could create local myout that takes the jmlputchar function as parameter...
typedef void (*Out)(int len, char c, char* s);

typedef int (*jmlputchartype)(int c);

static jmlputchartype jmlputchar;

// if s != NULL append it, 
// if len == 0 actually print c
// if len == 1 append c
void myout(int len, char c, char* s) {
    len = s ? (len > 0 ? len : strlen(s)) : len;
    // enough space?
    if (!out) {
        int sz = 1024;
        to = out = malloc(sz);
        memset(out, 0, sz);
        end = out + sz;
    }

    if (to + len >= end) {
        *to = 0;
        int sz = end-out + 1024 + len;
        char* nw = realloc(out, sz);
        fprintf(stderr, "[REALLOC %d]", sz);
        if (!nw) {
            fprintf(stderr, "ERRROR: realloc error!");
            exit(2);
        }
        to = to - out + nw;
        out = nw;
        end = out + sz;
    }

    if (s) {
        while (*s) myout(1, *s++, NULL);
    } else if (len == 1) { 
        *to++ = c;
    } else {
        jmlputchar(c);
    }
    *to = 0;
}

// TODO: make a linked list in FLASH
typedef struct FunDef {
    char* name;
    char* args;
    char* body;
} FunDef;

FunDef functions[SIZE] = {0};
int functions_count = 0;

// TODO: how to handle closures/RAM and GC?
void fundef(char* funname, char* args, char* body) {
    //printf("\nFUNDEF: %s[%s]>>>%s<<<\n", funname, args, body);
    if (functions_count > SIZE) { fprintf(stderr, "OUT OF FUNCTIONS!\n"); exit(1); }
    FunDef *f = &functions[functions_count++];
    // take a copy as the data comes from transient current state of program
    f->name = strdup(funname);
    f->args = strdup(args);
    f->body = strdup(body);
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

void fprintFuns(FILE* f) {
    int i;
    for(i = 0; i < functions_count; i++) {
        FunDef *fun = &functions[i];
        fprintf(f, "[macro %s%s%s]%s[/macro]\n",
                fun->name, strlen(fun->args) ? " " : "", fun->args, fun->body);
    }
}

// TODO: find temporary func/closures... [func ARGS]...[/func], only store in RAM and get rid of between runs...
void removefuns(char* s) {
    // remove [macro FUN ARGS]BODY[/macro] and doing fundef() on them
    char *m = s, *p;
    while (m = p = strstr(m, "[macro ")) {
        char* name = p + strlen("[macro ");
        char* spc = strchr(name, ' ');
        char* argend = strchr(name, ']');
        char* nameend = (spc < argend) ? spc : argend;
        if (nameend) *nameend = 0;
        *argend = 0;
        char* args = nameend ? nameend + 1 : argend + 1;
        if (args > argend) args = "";

        char* body = argend + 1;
        char* end = strstr(body, "[/macro]");
        if (!end) { fprintf(stderr, "\n%%ERROR: macro not terminated: %s\n", p); exit(3); }
        *end = 0; // terminate body
        fundef(name, args, body);

        end += strlen("[/macro]");
        memmove(m, end, strlen(end)+1);
    }
}

// We choose simple symetric encryption as we cannot secure the keys on
// embedded devices anyway.

// https://en.wikipedia.org/wiki/XXTEA
#define MX ((z>>5^y<<2) + (y>>3^z<<4) ^ (sum^y) + (k[p&3^e]^z))

// encrypt: btea(data, #longs (>1) keyhash (4 long))
// decrypt: btea(data, #longs (<1) keyhash (4 long))
//#include <stdint.h>
long btea(long* v, long n, long* k) {
    unsigned long z=v[n-1], y=v[0], sum=0, e, DELTA=0x9e3779b9;
    long p, q ;
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

#define HEX2INT(x) ({ int _x = toupper(x); _x >= 'A' ? _x - 'A' + 10 : _x - '0'; })

// At the current position in outstream get the body of funname
// and replace the actual arguments into positions of formals.
void funsubst(Out out, char* funname, char* args) {
    //fprintf(stderr, "\n(((%s,%s)))\n", funname, args);
    #define MAX_ARGS 10
    int argc = 0;
    char* farga[MAX_ARGS] = {0};
    int fargalen[MAX_ARGS] = {0};
    char* arga[MAX_ARGS] = {0};

    void parseArgs(char* fargs, char* args) {
        // fill in farga[] and fargalen[] from fargs
        while (*fargs) {
            while (*fargs == ' ') fargs++;
            if (!*fargs) break;
            farga[argc] = fargs;
            int len = 0;
            while (*fargs && *fargs != ' ') { fargs++; len++; }
            fargalen[argc] = len;
            argc++;
            if (argc > MAX_ARGS) { fprintf(stderr, "\n%%parseArgs MAX_ARGS too small!\n"); exit(4); }
        }
        // match extract each farg with actual arg
        int i = 0;
        while (*args == ' ') *args++ = 0;
        while (*args && argc && i < argc + (farga[argc-1][0] == '$')) {
            while (*args == ' ') *args++ = 0;
            arga[i] = args;
            while (*args && *args != ' ') args++;
            i++;
        }
    }
    #undef MAX_ARGS

    void outnum(int n) {
        int len = snprintf(NULL, 0, "%d", n);
        char x[len+1];
        snprintf(x, len+1, "%d", n);
        out(len, 0, x);
    }

    char* next() {
        char* a = args; args = NULL; // strtok will get only once
        char* r = strtok(a, " ");
        return r ? r : ""; // returning NULL may crash stuff...
    }
    int num() { return atoi(next()); }

    // either call out(...) and return
    // or fall through and
    // if s != NULL then this is output value
    // otherwise it stringifies the numeric value r
    // s = arg to make an ID function, next() to get next argument (modifies arg)
    int r = -4711;
    char* s = NULL;
    FunDef *f = findfun(funname);
    if (f) {
        char* body = f->body;
        char* fargs = f->args;

        parseArgs(fargs, args);
        
        char c;
        while (c = *body) {
            if (c == '$' || c == '@' ) {
                // find named prefix if match formal argument name
                int i = 0;
                while (i < argc && strncmp(body, farga[i], fargalen[i])) i++;

                // substitute
                if (i < argc) {
                    out(-1, 0, arga[i]);
                    body += fargalen[i];
                } else {
                    printf("\n%%Argument: %s not found!\n", strtok(body, " "));
                }
            } else {
                out(1, *body++, NULL);
            }
        }
        return;
    } 
    else if (!strcmp(funname, "inc")) r = num() + 1;
    else if (!strcmp(funname, "dec")) r = num() - 1;
    else if (!strcmp(funname, "-")) r = num() - num();
    else if (!strcmp(funname, "/")) r = num() * num();
    else if (!strcmp(funname, "%")) r = num() % num();
    else if (!strcmp(funname, ">")) r = num() > num();
    else if (!strcmp(funname, "<")) r = num() < num();
    else if (!strcmp(funname, ">=")) r = num() >= num();
    else if (!strcmp(funname, "<=")) r = num() <= num();
    else if (!strcmp(funname, "=")) r = num() == num();
    else if (!strcmp(funname, "!=")) r = num() != num();
    else if (!strcmp(funname, "+")) { r = 0; char* x; while (*(x = next())) r += atoi(x); }
    else if (!strcmp(funname, "*")) { r = 1; char* x; while (*(x = next())) r *= atoi(x); }
    else if (!strcmp(funname, "iota")) {
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
            out(1, ' ', NULL);
        }
        return;
    }
    else if (!strcmp(funname, "if")) {
        int e = num(); char *thn = next(), *els = next();
        thn = thn ? thn : "";
        els = els ? els : "";
        s = e ? thn : els;
    }
    else if (!strcmp(funname, "empty")) { char* x = next(); r = !x ? 1 : !strcmp(x, ""); }
    else if (!strcmp(funname, "equal")) r = strcmp(next(), next()) == 0;
    else if (!strcmp(funname, "cmp")) r = strcmp(next(), next());
    // we can modify the input strings, as long as they get shorter...
    else if (!strcmp(funname, "lower")) { char* x = s = args; while (*x = tolower(*x)) x++; }
    else if (!strcmp(funname, "upper")) { char* x = s = args; while (*x = toupper(*x)) x++; }
    else if (!strcmp(funname, "length")) { r = 0; while (*next()) r++; }
    else if (!strcmp(funname, "nth")) { int n = num(); s = ""; while (n-- > 0) s = next(); }
    else if (!strcmp(funname, "map")) {
        char* fun = next();
        char* x = NULL;
        while (*(x = next())) {
            out(1, '[', NULL);
            out(-1, 0, fun);
            out(1, ' ', NULL);
            out(-1, 0, x);
            out(1, ']', NULL);
            out(1, ' ', NULL);
        }
        return;
    } else if (!strcmp(funname, "after")) {
        char* x = args;
        char* find = next();
        x += strlen(find) + 1;
        char* f = strstr(x, find);
        if (f) out(-1, 0, f + strlen(find));
        return;
    } else if (!strcmp(funname, "before")) {
        char* x = args;
        char* find = next();
        x += strlen(find) + 1;
        char* f = strstr(x, find);
        if (!f) return;
        *f = 0;
        out(-1, 0, x);
        return;
    } else if (!strcmp(funname, "field")) { // extract simple xml, one value from one field
        // LOL: 'parsing' "xml" by char*!
        char* x = args;
        char* name = next();
        char needle[strlen(name) + 2];
        *needle = 0;
        strcat(needle, "<");
        strcat(needle, name);
        // can be terminated by ' ' or '>' hmmm.
        x += strlen(name) + 1;
        while (*x) {
            char* f = strstr(x, needle);
            if (!f) return;
            f += strlen(needle);
            if (*f == '>' || *f == ' ') {
                char* start = strchr(f, '>');
                if (start) {
                    start++;
                    char endneedle[strlen(name) + 4];
                    *endneedle = 0;
                    strcat(endneedle, "</");
                    strcat(endneedle, name);
                    strcat(endneedle, ">");
                    char* end = strstr(start, endneedle);
                    if (end) {
                        *end = 0;
                        out(1, '{', NULL);
                        out(-1, 0, start);
                        out(1, '}', NULL);
                        return;
                    }
                }
            }
            x = f;
        }
        return;
    } else if (!strcmp(funname, "concat")) {
        s = args; char* d = args;
        // essentially it concats strings by removing spaces
        // and copying the rest of string over the space
        while (*args) {
            while (*args && *args != ' ') *d++ = *args++;
            while (*args && *args == ' ') args++;
        }
        *d = 0;
    } else if (!strcmp(funname, "split")) {
        char* x = args;
        char* needle = next();
        x += strlen(needle) + 1;
        while (*x) {
            char* f = strstr(x, needle);
            if (!f) break;
            *f = 0;
            // TODO: since output is smaller (replace needle with ' '!)
            out(-1, 0, x);
            out(1, ' ', NULL);
            x = f + strlen(needle);
        }
        out(-1, 0, x);
        return;
    } else if (!strcmp(funname, "decode")) {
        char* x = s = next();
        while (*x) {
            if (*x == '+') *x = ' ';
            if (*x == '/') *x = ' ';
            if (*x == '%') {
                *x = HEX2INT(*(x+1)) * 16 + HEX2INT(*(x+2));
                memmove(x+1, x+3, strlen(x+3) + 1);
            }
            x++;
        }
    } else if (!strcmp(funname, "encrypt")) {
        char hex[] = "0123456789ABCDEF";
        char* key = "1234123412341234";
        long k[4];
        memset(k, 0, 16);
        int kl = strlen(key);
        memcpy(k, key, kl > 16 ? 16 : kl);

        int l = strlen(args);
        int n = (l+3)/4;
        // we need to pad to at least 8 bytes, for n == 1 no encryption!
        if (n < 2) n = 2;
        long data[n];
        memset(data, 0, n*4);
        memcpy(data, args, l+1);
        btea(data, n, k);

        char* d = (char*)data;
        int i;
        out(1, '{', NULL);
        for(i = 0; i < n*4; i++) {
            unsigned char c = *d++;
            out(1, hex[c / 16], NULL);
            out(1, hex[c % 16], NULL);
        }
        out(1, '}', NULL);
        return;
    } else if (!strcmp(funname, "decrypt")) {
        char* key = "1234123412341234";
        long k[4];
        memset(k, 0, 16);
        int kl = strlen(key);
        memcpy(k, key, kl > 16 ? 16 : kl);

        // decode hex inplace
        char* d = args;
        char* p = args+1; // skip '{'
        while (*p && *p != '}') {
            char a = *p++;
            char b = *p++;
            *d++ = HEX2INT(a) * 16 + HEX2INT(b);
        }
        *d++ = 0;
        
        // TODO: refactor, same as above?
        int n = (d - args - 1)/4;
        long data[n];
        memset(data, 0, n*4);
        memcpy(data, args, n*4);
        btea(data, -n, k);

        // copy it out
        d = (char*)data;
        int i;
        for(i = 0; i < n*4; i++) {
            if (!*d) break;
            out(1, *d++, NULL);
        }
        return;
    } else if (!strcmp(funname, "data")) {
        s = args;
        char* id = next();
        if (!id) return;
        char* data = s + strlen(id) + 1;
        char name[strlen(id)+1+5];
        name[0] = 0;
        strcat(name, "data-");
        strcat(name, id);
        fundef(name, "", data);
        s = data;
    } else if (!strcmp(funname, "funcs")) {
        int i;
        for(i = 0; i < functions_count; i++) {
            out(-1, 0, functions[i].name);
            out(1, ' ', NULL);
        }
        return;
    } else {
        out(-1, 0, "<font color=red>%(FAIL:");
        out(-1, 0, funname);
        out(1, ' ', NULL);
        out(-1, 0, args);
        out(-1, 0, ")%</font>");
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
    
    removefuns(s);

    int substcount = 0;
    char* exp = NULL;
    char* funend = NULL;
    char c;
    int doprint = 1;
    while (c = *s) {
        if (c == '[') { // "EVAL"
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
            if (c == ']') { // "APPLY"
                char* p = exp;
                char* args = NULL;

                if (!funend) {
                    funend = s;
                    args = "";
                } else {
                    args = funend + 1;
                }

                *funend = 0;
                char* funname = p;
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
        // next
        s++;
    }
    // end
    //*to = 0;
    return substcount;
}

// assumes one mallocated string in, it will be freed
// no return, just output on "stdout"
int oneline(char* s, jmlputchartype putt) {
    clock_t start = clock();
    // temporary change output method
    void* stored = jmlputchar;
    jmlputchar = putt;

    do {
        if (!run(s, myout)) break;

        free(s);
        s = out;
        end = to = out = NULL;
    } while (1);
    printf("%s", out); fflush(stdout);

    free(s);
    free(out);
    end = to = out = NULL;
    
    // restore output method
    jmlputchar = stored;
    int t = (clock() - start) * (1000000 / CLOCKS_PER_SEC); // noop!
    fprintf(stderr, "...(%d us)...", t);
    return t;
}

char* freadline(FILE* f) {
    size_t len = 0;
    char* ln = NULL;
    int n = getline(&ln, &len, f);
    if (n < 0 && ln) { free(ln); ln = NULL; }
    return ln;
}

static void jmlheader(char* buff, char* method, char* path) {
    //printf("HEADER.ignored: %s\n", buff);
}

static void jmlbody(char* buff, char* method, char* path) {
    //printf("BODY.ignored: %s\n", buff);
}

int doexit = 0;

static void jmlresponse(int req, char* method, char* path) {
    printf("------------------------------ '%s' '%s'\n\n", method, path);
    if (!strcmp("/exit", method)) {
        doexit = 1;
        return;
    }

    // args will point to data after '?' or ' '
    char* args = strchr(path, '?');
    if (!args) args = strchr(path, ' ');
    if (!args) args = strchr(path+1, '/');
    if (!args) {
	args = "";
    } else {
        *args = 0;
        args++;
    }

    // skip '/' if '/foo' not found
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
    myout(-1, 0, path);
    myout(1, ' ', NULL);

    // TODO: unsafe to allow call of any function, maybe only allow call "/func" ?
    if (strchr(args, '=')) { // url on form /fun?var1=value1&var2=value2
        // TODO: could do a decode that extracts parameters and match to fargs!
        // for now, just let function decode and extract itself!
        myout(-1, 0, args);
    } else { // url on form /fun?arg1+arg2
        myout(-1, 0, "[decode ");
        myout(-1, 0, args);
        myout(1, ']', NULL);
    }

    myout(1, ']', NULL);
    
    printf("OUT=%s<\n", out);

    int putt(int c) {
        char ch = c;
        return write(req, &ch, 1);
    }

    // make a copy
    char* line = strdup(out);
    if (out) free(out); end = to = out = NULL;

    oneline(line, putt);

    // line is freed by oneline, output putt
}

int main(int argc, char* argv[]) {
    jmlputchar = putchar;

    // parse argument
    int argi = 1;
    int web = 0;
    if (argc > argi && !strcmp(argv[argi++], "-w")) {
        web = argc > argi ? atoi(argv[argi]) : 0;
        if (web > 0)
            argi++;
        else
            web = 1111;
    }
    char* state = argc > argi ? argv[argi++] : "jml.state";
    fprintf(stderr, "web=%d state=%s\n", web, state);

    int putstdout(int c) {
        return fputc(c, stdout);
    }

    FILE* f = fopen(state, "r");
    while (f && !feof(f)) {
        // TODO: how to handle macro def/invocations over several lines?
        char* line = freadline(f);
        if (!line) break;

        oneline(line, putchar);
    }
    if (out) { free(out); end = to = out = NULL; }
    if (f) fclose(f);

    if (web) {
        // web server
        int www = httpd_init(web);
        if (www < 0 ) { printf("ERROR.errno=%d\n", errno); return -1; }
        printf("\n%%Webserver started on port=%d\n", web);
        doexit = 0;
        while (!doexit) {
            httpd_next(www, jmlheader, jmlbody, jmlresponse);
        }
    } else {
        // TODO: make it part of non-blocking loop!
        char* line = freadline(stdin);
        if (line) {
            // TODO: how to handle macro def/invocations over several lines?
            oneline(line, putchar);
        }
    }

    // TODO: make it append and only write "new" stuff
    f = fopen(state, "w");
    fprintFuns(f);
    fclose(f);
} 
