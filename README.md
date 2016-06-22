# jml - a "minimal" operating system

TODO: find a better name?

More useless than (Urbit)[http://urbit.org/]!

To be used for ESP8266 as a minimal functional internet OS with
"memory", see (immutable operating
system)[https://hn.algolia.com/story/7166173/an-immutable-operating-system?query=&sort=byPopularity&prefix&page=0&dateRange=all&type=all].

## Goal

Simple online programmable virtual physical computer with built in persistent "memory".

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
- * % inc dec > < >= <= = !=
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
- [cmp A B] => (A cmp B)
- lower upper
- [length 1 2 3] = 3
- [length 11 22 33 44] = 4

Lists
- [nth 2 11 22 33] => 22
- [map F A B C ...] => [F A] [F B] [F C] [map F ...]

Strings
- [after X abcXzy] => zy
- [before X abcXzy] = abc
- [field name ksajf; sadflk dsaflk <name c='foo'>FISH</name> sdfl sadf asdfdsa] => {FISH}
- [concat A B C D ...] => ABCD...
- [split a aAaBBaAa] => A BB A

WEB, decode URL
- [decode foo+bar%2b%25] => foo bar+%

Security/Encryption/TEA
- [encrypt FOOBAR] => {39CE0CD92EEBACC8}
- [decrypt {39CE0CD92EEBACC8}] => FOOBAR
- [encrypt-eval (+ 3 4)] => {D9403DC570A74AF1}
- [decrypt {D9403DC570A74AF1}] => (+ 3 4)
- [decrypt-eval {D9403DC570A74AF1}] => (+ 3 4) => 7

Storage/persistent/database
- [data firstname Peter] .. [data-firstname] => Peter
- [funcs] => user define func names

Failure
- [xyz sadfasdf] => (%FAIL: xyz sadfasdf)

### Alternative Universe Inspired Readings 

- (unikernel listings)[http://unikernel.org/projects/]
  TODO: add myself?
  
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

- how to build and bootstrap your own os (on PC arch) (link)[http://www.brokenthorn.com/Resources/]
- http://www.menuetos.net/index.htm
- http://www.jamesmolloy.co.uk/tutorial_html/index.html
- http://www.osdever.net/FreeVGA/home.htm
- https://github.com/ReturnInfinity/BareMetal-OS
- http://wiki.osdev.org/Main_Page
- http://home.in.tum.de/~hauffa/slides.pdf

## Urbit summary (and features I'd like have?)

(urbit intro by third party)[http://alexkrupp.typepad.com/sensemaking/2013/12/a-brief-introduction-to-urbit.html]

- decentralized computing platform
- clean-slate OS
- personal server
- persistent virtual computer
- you own it, trust and control
- "Urbit is perhaps how you'd do computing in a post-singularity world, where computational speed and bandwidth are infinite, and what's valuable is security, trust, creativity, and collaboration. It's essentially a combination of a programming language, OS, virtual machine, social network, and digital identity platform."
- UDP
- reputation based social network (?)
- event logging
