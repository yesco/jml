// idea:
// "one heap"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 1024

// TODO: move this up
char* out = NULL;
char* to = NULL;
char* end = NULL;

typedef void (*PutChar)(int len, char c, char* s);

// if s != NULL append it, 
// if len == 0 actually print c
// if len == 1 append c
void myputchar(int len, char c, char* s) {
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
            printf("ERRROR: realloc error!");
            exit(2);
        }
        to = to - out + nw;
        out = nw;
        end = out + sz;
    }

    if (s) {
        while (*s) myputchar(1, *s++, NULL);
    } else if (len == 1) { 
        *to++ = c;
    } else {
        putchar(c);
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
    if (functions_count > SIZE) { printf("OUT OF FUNCTIONS!\n"); exit(1); }
    FunDef *f = &functions[functions_count++];
    // take a copy as the data comes from transient current state of program
    f->name = strdup(funname);
    f->args = strdup(args);
    f->body = strdup(body);
}

FunDef* findfun(char* funname) {
    FunDef *f = &functions[0];
    while (f->name) {
        if (!strcmp(funname, f->name)) return f;
        f++;
    }
    return NULL;
}

// TODO: find macros to FunDef above...  [macro FUN ARGS]...[/macro]
// TODO: find temporary func/closures... [func ARGS]...[/func], only store in RAM and get rid of between runs...
// and ... [data ID DATA] [get ID] [history ID] gives id: [dir MATCHID] [find DTA] [search DTA]
// modifies input by removing defs.. in place
void removefuns(char* s) {
    char *m = s, *p;
    while (m = p = strstr(m, "[macro ")) {
        char* name = p + strlen("[macro ");
        char* spc = strchr(name, ' ');
        char* argend = strchr(name, ']');
        char* nameend = (spc < argend) ? spc : argend;
        *nameend = 0;
        *argend = 0;
        char* args = nameend + 1;
        if (args > argend) args = "";

        char* body = argend + 1;
        char* end = strstr(body, "[/macro]");
        if (!end) { printf("\n%%ERROR: macro not terminated: %s\n", p); exit(3); }
        *end = 0; // terminate body
        fundef(name, args, body);

        end += strlen("[/macro]");
        memmove(m, end, strlen(end)+1);
    }
}

void funsubst(PutChar out, char* funname, char* args) {
    //printf("<<<%s,%s>>>", funname, args);

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
        }
        // match extract each farg with actual arg
        int i = 0;
        while (*args == ' ') *args++ = 0;
        while (*args && i < argc) {
            while (*args == ' ') *args++ = 0;
            arga[i] = args;
            while (*args && *args != ' ') args++;
            i++;
        }
    }

    void outnum(int n) {
        int len = snprintf(NULL, 0, "%d", n);
        char x[len+1];
        snprintf(x, len+1, "%d", n);
        out(len, 0, x);
    }

    char* next() {
        char* a = args; args = NULL; // assign once
        if (argc > 10) { printf("\n%%next(): run out of arga position! too many arguments!\n"); exit(4); }
        return strtok(a, " ");
    }
    int num() { return atoi(next()); }

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
    else if (!strcmp(funname, "+")) r = num() + num();
    else if (!strcmp(funname, "-")) r = num() - num();
    else if (!strcmp(funname, "*")) r = num() * num();
    else if (!strcmp(funname, "/")) r = num() * num();
    else if (!strcmp(funname, "%")) r = num() * num();
    else if (!strcmp(funname, ">")) r = num() > num();
    else if (!strcmp(funname, "<")) r = num() < num();
    else if (!strcmp(funname, ">=")) r = num() >= num();
    else if (!strcmp(funname, "<=")) r = num() >= num();
    else if (!strcmp(funname, "=")) r = num() == num();
    else if (!strcmp(funname, "!=")) r = num() != num();
    else if (!strcmp(funname, "iota")) {
        int a = num(), i = 0;
        while (i < a) {
            outnum(i++);
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
    else if (!strcmp(funname, "lower")) { s = next(); char* x = s; while (*x = tolower(*x)) x++; }
    else if (!strcmp(funname, "concat")) {
        s = args; char* d = args;
        // essentially it concats strings by removing spaces
        while (*args) {
            while (*args && *args != ' ') *d++ = *args++;
            while (*args && *args == ' ') args++;
        }
        *d = 0;
    } else if (!strcmp(funname, "data")) {
        s = args;
        char* id = next();
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
        out(-1, 0, "%(FAIL:");
        out(-1, 0, funname);
        out(-1, 0, args);
        out(-1, 0, ")%");
        return;
    }

    if (s) {
        out(-1, 0, s);
    } else {
        outnum(r);
    }
}

int run(char* start, PutChar out) {
    char* s = start;
    
    removefuns(s);

    int substcount = 0;
    char* exp = NULL;
    char* funend = NULL;
    char c;
    int doprint = 1;
    while (c = *s) {
        if (c == '[') {
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
            if (c == ']') {
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
    *to = 0;
    return substcount;
}

void main() {
    char* start = NULL;
    size_t len = 0;
    int ln = getline(&start, &len, stdin);
    //start = strdup("foo [[concat up per] fie] fum");
    printf("%s\n", start);
    do {
        if (!run(start, myputchar)) break;

        free(start);
        start = out;
        out = NULL;
    } while (1);
    printf("\n");
    free(start);
    free(out);
} 

