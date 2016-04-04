// idea:
// "one heap"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* start = NULL;

#define SIZE 1024

char* out = NULL;
char* end = NULL;

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

void funsubst(char** pto, char* funname, char* args) {
    //printf("<<<%s,%s>>>", funname, args);

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
        int len = snprintf(*pto, 255 /*TODO*/, "%(FAIL:%s %s)", funname, args);
        *pto += len;
        return;
    }

    if (s) {
        int len = snprintf(*pto, 255 /*TODO*/, "%s", s);
        *pto += len;
    } else {
        int len = snprintf(*pto, 255 /*TODO*/, "%d", r);
        *pto += len;
    }
}

int run() {
    char* to = out;
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
                while (exp < s) {
                    *to++ = *exp;
                    exp++;
                }
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
                char* x = to;
                funsubst(&to, funname, args);
                substcount++;

                exp = NULL;
                funend = NULL;
            } else if (!funend && c == ' '){
                funend = s;
            } else {
                // expression skip till ']'
            }
        } else if (doprint) {
            putchar(c);
        } else { // retain for next loop
            *to++ = c;
        }
        // next
        s++;
    }
    // end
    *to = 0;
    return substcount;
}

void main() {
    start = strdup("foo [[concat up per] fie] fum");
    printf("%s\n", start);
    do {
        out = malloc(SIZE);//calloc(SIZE, 0);
        memset(out, 0, SIZE);
        end = out + SIZE; // TODO: not used...
        //printf(" IN:::%s:::\n", start);
        //printf("   :::");
        if (!run()) break;
        //printf("\nOUT:::%s:::\n", out);
        free(start);
        start = out;
    } while (1);
    printf("\n");
    free(start);
    free(out);
} 

