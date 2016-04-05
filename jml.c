// idea:
// "one heap"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 1024

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
}

typedef struct FunDef {
    char* name;
    char* args;
    char* body;
} FunDef;

// TODO: make a linked list in FLASH
FunDef functions[SIZE] = {0};
int functions_count = 0;

// TODO: how to handle closures/RAM and GC?
void fundef(char* funname, char* args, char* body) {
    if (functions_count > SIZE) { printf("OUT OF FUNCTIONS!\n"); exit(1); }
    FunDef *f = &functions[functions_count++];
    // take a copy as the data comes from transient current state of program
    f->name = strdup(funname);
    f->args = strdup(args);
    f->body = strdup(body);
}

void removefuns() {
}

void funsubst(PutChar out, char* funname, char* args) {
    //printf("<<<%s,%s>>>", funname, args);

    void outnum(int n) {
        int len = snprintf(NULL, 0, "%d", n);
        char x[len+1];
        snprintf(x, len+1, "%d", n);
        out(len, 0, x);
    }

    char* next() { char* a = args; args = NULL; return strtok(a, " "); }
    int num() { return atoi(next()); }

    int r = -4711;
    char* s = NULL;
    if (0) ;
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
    // we can modify the input strings, as long as they get shorter...
    else if (!strcmp(funname, "upper")) { s = next(); char* x = s; while (*x = toupper(*x)) x++; }
    else if (!strcmp(funname, "lower")) { s = next(); char* x = s; while (*x = tolower(*x)) x++; }
    else if (!strcmp(funname, "concat")) {
        s = args; char* d = args;
        while (*args) {
            while (*args && *args != ' ') *d++ = *args++;
            while (*args && *args == ' ') args++;
        }
        *d = 0;
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
    }
}

int run(char* start, PutChar out) {
    char* s = start;
    
    removefuns();

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
                if (!funend) funend = s;
                *funend = 0;
                char* funname = p;
                p = funend + 1;
                *s = 0;
                char* args = p;
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

