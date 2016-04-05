# jml

More useless than (Urbit)[http://urbit.org/]!

To be used for ESP8266 as a minimal functional internet OS with
"memory", see (immutable operating
system)[https://hn.algolia.com/story/7166173/an-immutable-operating-system?query=&sort=byPopularity&prefix&page=0&dateRange=all&type=all].

## simple string based evaluation

    foo => foo
    foo [+ 3 4] bar => foo 7 bar
    foo [upper bar] fie => foo BAR fie
    [[concat up per] fie] = FIE

## super eager evaluation

The interpreter is very simple: it just eagerly evaluates any inner
[fun par] expression where fun and par themselves have no function
calls. I.e., the innermost [ ] expressions are evaluated successively,
the result is achieved by replacing the expression with the body where
the formal parameter names have been substituted by the actual
values. Parameters are space delimited.

 