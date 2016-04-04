// idea:
// "one heap"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* start = NULL;

#define SIZE 1024

char* out = NULL;
char* end = NULL;

void funsubst(char** pto, char* funname, char* args) {
    printf("(((%s,%s)))", funname, args);
    char* to = *pto;
    int len = snprintf(*pto, 255 /*TODO*/, "%s", "plus");
    *pto += len;
}

void run() {
    char* to = out;
    char* s = start;
    
    char* exp = NULL;
    char* funend = NULL;
    char c;
    while (c = *s) {
        if (c == '[') {
            if (exp) {
                exp--;
                while (exp < s) {
                    putchar(*exp);
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
                funsubst(&to, funname, args);

                exp = NULL;
                funend = NULL;
            } else if (!funend && c == ' '){
                funend = s;
            } else {
                // expression;
            }
        } else {
            putchar(c);
            *to++ = c;
        }
        // enxt
        s++;
    }
    // end
    *to = 0;
}

void main() {
    start = strdup("foo [[bar xxx] fie] fum");
    out = calloc(0, SIZE);
    end = out + SIZE;
    printf(" IN>>>%s<<<\n", start);
    printf("   >>>");
    run();
    printf("\nOUT>>>%s<<<\n", out);
} 

