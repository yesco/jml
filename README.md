# jml - a useful web minimal unikernel operating system

TODO: rename to [MagniOS](https://en.wikipedia.org/wiki/M%C3%B3%C3%B0i_and_Magni)

## Goal

Simple online programmable virtual physical computer with built in persistent "memory".

## Woa!?

This is a [First Principle's](https://en.wikipedia.org/wiki/First_principle) project.
It means it starts from "scratch" with minimal requirements. Elon Musk uses this as
his [innovation principle](http://99u.com/workbook/20482/how-elon-musk-thinks-the-first-principles-method).

## Requirements

C (gcc) compiler, and simple unix style standard library calls.
For security no external libraries are used.

## Why

The idea of a minimal OS is coming up again, maybe recently (now is
2016) than the last 30 years. We have docker, and various
virtualization environments. It's funny to simulate an actual once
existing hardware computer - mostly just a PC. However, there are
other approaches, more minimal which are single language based. It is
sometimes called a library OS. For example RTOS for ESP8266
[esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos) can be
seen as such. It provides a small implementation of most of the
libraries and system calls you'd expect on a POSIX/linux style
computer. It is limited to a single process, so no fork. It does
provide it's own threading/tasks and semaphore/messaging
system. However, it doesn't provide infrastructure to think "above a
single computer".

## What is it?

This project is an attempt to provide a small relatively simple C
program implementing a *portable small language* onto such an
invironment. Of course, for more complicated tasks a multitude of
such distributed instances would be able to cooperate in order to solve
a bigger problem.

## Status

Status: More "useless" than [Urbit](http://urbit.org/)!

## Outline of actions/TODO/DONE

- DONE: small simple small language interpreter in C
- what's the minimal set of functions needed?
- connectivity, receive requests/send requests (messaging)
- logical + physical addressing something like [Kademlia](https://en.wikipedia.org/wiki/Kademlia)
- p2p filesystem w some redundancy - [PAST](https://en.wikipedia.org/wiki/Pastry_(DHT))
- p2p pubsub - [SCRIBE](https://en.wikipedia.org/wiki/Pastry_(DHT))
- callback verification [Magic cookie](https://en.wikipedia.org/wiki/Magic_cookie)
- https://en.wikipedia.org/wiki/Merkle_tree
- cheap hashes https://en.wikipedia.org/wiki/Tiger_(cryptography)

## How to run

### command line

    ./run
    
### batch

This allows a single command, or several to be run and output captured.

    echo "[concat a b c]" | ./run 2>/dev/null

### web

To run it as a webserve on port 1111, or as given use the command below.
Connect to it as [localhost:1111/](localhost:1111/).

    ./run -w [PORT]

### debugging using tracing

    unix> echo "[+ [map inc 1 2 3]]" | ./run -t
    >>>[+ [map inc 1 2 3]]<<<
    >>>[+ [inc 1] [inc 2] [inc 3] ]<<<
    >>>[+ 2 3 4 ]<<<
    >>>9<<<
    9
    

    unix> echo '[macro fac $n][* [iota 1 $n]][/macro]' | ./run
      [appended fac to file]
    
    unix> echo "[fac 5]" | ./run -t
    >>>[fac 5]<<<
    >>>[* [iota 1 5]]<<<
    >>>[* 1 2 3 4 5 ]<<<
    >>>120<<<
    120


# Language

## simple string based evaluation

    foo => foo
    foo [+ 3 4] bar => foo 7 bar
    foo [upper bar] fie => foo BAR fie
    [[concat up per] fie] = FIE

Note, this language has a "different" if

    [if 0 1 2] => 2
    [if 1 1 2] => 1
    [if [< 3 4] smaller bigger] => smaller

however, not, it's not lazy eval or special form:

    [macro foo]11 22[/macro]
    [macro bar]2 1[/macro]

    [if 0 [foo] [bar]] => 20

    ???

    [if 0 [foo] [bar]] =>
    [if 0 11 22 [bar]] =>
    [if 0 11 22 2 1]] =>
    22 !!! second argument returned

How to do IFFFF THEN?

    [[if 0 foo bar]] => 2 1
    [[if 1 foo bar]] => 11 22

So... you can do selection of names to expand. Switch style:

    [macro en-0]zero[/macro]
    [macro en-1]one[/macro]
    [macro en-2]two[/macro]
    [macro en-3]three[/macro]
    [macro en-4]four[/macro]
    [macro en-5]five[/macro]
    [macro en-6]six[/macro]
    [macro en-7]seven[/macro]
    [macro en-8]eight[/macro]
    [macro en-9]nine[/macro]
    [macro en-10]ten[/macro]
    [macro en-][/macro]
    
    [macro en $digit][en-$digit] [/macro]

    [macro english @digits][map en @digits][/macro]

    [macro english-rec $digit @digits][en-$digit] [[if [empty @digits] ignore english-rec] @digits][/macro]

## help

from command line:

    [help]

from web browser

    unix> ./jml -w

    http:localhost:1111/help


## super eager evaluation

The interpreter is very simple: it just eagerly evaluates any inner
[fun par] expression where fun and par themselves have no function
calls. I.e., the innermost [ ] expressions are replaced successively,
the result is achieved by replacing the expression with the body where
the formal parameter names have been substituted by the actual
values. Parameters are space delimited.

## efficiency

Bah... computers are fast! Seriously, we're generating HTML and serving
it on the net, so what's fast anyway?

## features

- active messaging
- reactive, single user space, no address sharing
- no processes, or tasks, single threaded
- persistent data/program/functions once "uploaded" (TODO)
- simple evaluation mechanism
- http communication (TODO)

## built-in functions

Math
- % inc dec > < >= <= = !=
- [+ 1 2 3 4] => 10
- [* 1 2 3 4] => 24
- [iota 1 3] => 1 2 3
- [iota 0 10 2] 

Test
- [if 0 THIS not] => THIS
- [if X not THAT] => THAT
- [empty] =>1
- [empty ...] => 0
- [empty    ] => 1
- [equal A B] => 0
- [cmp A B] => 1
- [cmp B A] => -1
- [cmp A A] => 0
- lower upper
- [length 1 2 3] => 3
- [length 11 22 33 44] => 4
- [bytes] => 0
- [bytes 1 2 3] => 5
- [bytes 1 2 3 ] => 6
- [bytes  1 2 3 ] => 7

Lists
- [nth 2 11 22 33] => 22
- [map F A B C ...] => [F A] [F B] [F C] [map F ...]
- [first A B] => A
- [second A B C] => B
- [third A B C D] => C
- [rest A B C D] => B C D

Strings
- [after X abcXzy] => zy
- [before X abcXzy] = abc
- [field name ksajf; sadflk dsaflk <name c='foo'>FISH</name> sdfl sadf asdfdsa] => {FISH}
- [concat A B C D ...] => ABCD...
- [concat A\ B C D ...] => A BCD...
- [concat [concat a\ b c]] => A BCD...
- [split a aAaBBaAa] => A BB A

WEB, decode URL
- [decode foo+bar%2b%25] => foo bar+%

Storage/persistent/database
- [data firstname Peter] .. [data-firstname] => Peter
- [data] =? firstname ...
- [funcs] => user define func names
- [fargs macroname] -> $a $b @foo
- [fbody macroname] -> foo $a fie $b fum: @foo

Failure
- [xyz sadfasdf] => (%FAIL: xyz sadfasdf)

### Note on quoting

Since everything works by substitution (close to lambda calculus?), the need for
quoting is low. This is normally called interpolation in languages as Perl. In JML
it's a little different as the interpolated string is continously interpolated by
substitutions of [fun ...] "calls". However, as it's uses '[', ']', and ' ' as significant
these now instead need to be quoted. LOL. However, for the domain of email. This is "convenient".

This is how to quote these characters
- A\ B is interpreted as one parameter in calling, see concat above
- \[+ 3 4\] will print [+ 3 4] and not 7

#### Safety, "SQL Injection"

In order to not allow injection, certain "web" characters are quoted on "input" from
the web. These are: < > \[ \] &amp; ' "

#### Unicode

TODO: Haha, come again? Don't you know the world consists of bits and bytes?

Seriously, the web is multilingular, and I travel in china, use swedish so UTF-8 is a reasonable requirement.

[UTF-8](https://en.wikipedia.org/wiki/UTF-8):

"And ASCII bytes do not occur when encoding non-ASCII code points into UTF-8, making UTF-8 safe to use within most programming and document languages that interpret certain ASCII characters in a special way, e.g. as end of string."

### Security/Encryption/(TEA)[https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm]
- [encrypt FOOBAR] => {39CE0CD92EEBACC8}
- [decrypt {39CE0CD92EEBACC8}] => FOOBAR
- [encrypt-eval (+ 3 4)] => {D9403DC570A74AF1}
- [decrypt {D9403DC570A74AF1}] => (+ 3 4)
- [decrypt-eval {D9403DC570A74AF1}] => (+ 3 4) => 7

Note on "security", it's just XXTEA which can only provide nominal
security in access. They keys aren't protected and if one has access
to the physical device would be able to extract the key. However,
each instance should have it's own key and the person communicating with it need it too.
This means; It's protected against middle-man attack. Each unikernel/device should have
it's own key. It may be possible to use the key to only allow the owner of
that key to modify and run any code. Others would only be allowed to interact
using the public defined macro functions, like [macro /foo]...

Encrypted data becomes HEX coded, thus will at use at least the double
amount of bytes.

#### Termination

Currently, if you loop forever by recursion there is no limit. One could add
a max-substitution count, or time that would terminate the current running request.

### Alternative Universe Inspired Readings 

- (unikernel listings)[http://unikernel.org/projects/]
  TODO: add myself?
  
- [immutable operating system](https://hn.algolia.com/story/7166173/an-immutable-operating-system?query=&sort=byPopularity&prefix&page=0&dateRange=all&type=all)

- HackerNews: [Alan Kay has agreed to do an AMA today](https://news.ycombinator.com/item?id=11939851)
  This is an amazing discussion. 
  
- https://en.wikipedia.org/wiki/Amorphous_computing

- Phatom_OS - a managed code on object level rather than process level
  (no pointer arithmetic). Cheap IPC. Persistence at core, live forever, no file system needed,
  (link)[https://en.wikipedia.org/wiki/Phantom_OS]

- Active Messaging, where is a messaging passing system where messages
  not only contain data but also the name of a userspace handler to be
  executed upon arrival (i.e. a function call!), sometimes it can
  carry the actual code in the message...  (active
  messaging)[https://en.wikipedia.org/wiki/Active_message]

- Urbit OS; clean slate OS, packet protocol, pure functional language, deterministic OS, ACID database,
  versioned filesystem, a webserver, global PKI, runs a frozen combinator interpreter, opaque computing
  and communcation layer ontop of unix and internet. The user owns his own general purpose server or "planet".
  Repeatable computing. 
  (link)[http://urbit.org/docs/theory/whitepaper]

- http://vpri.org/html/work/ifnct.htm

- http://www.otonomos.com/

### Readings about x86 style OSes

- how to build and bootstrap your own os (on PC arch) [link](http://www.brokenthorn.com/Resources/)
- http://www.menuetos.net/index.htm
- http://www.jamesmolloy.co.uk/tutorial_html/index.html
- http://www.osdever.net/FreeVGA/home.htm
- https://github.com/ReturnInfinity/BareMetal-OS
- http://wiki.osdev.org/Main_Page
- http://home.in.tum.de/~hauffa/slides.pdf
- crochet [squeak/3d collaboration space](http://worrydream.com/refs/Smith%20-%20Croquet%20-%20A%20Collaboration%20System%20Architecture.pdf)

## Urbit summary (and features I'd like have?)

[urbit intro by third party](http://alexkrupp.typepad.com/sensemaking/2013/12/a-brief-introduction-to-urbit.html)

- decentralized computing platform
- clean-slate OS
- personal server
- persistent virtual computer
- you own it, trust and control
- "Urbit is perhaps how you'd do computing in a post-singularity world, where computational speed and bandwidth are infinite, and what's valuable is security, trust, creativity, and collaboration. It's essentially a combination of a programming language, OS, virtual machine, social network, and digital identity platform."
- UDP
- reputation based social network (?)
- event logging
