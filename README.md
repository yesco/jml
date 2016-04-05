# jml - a "minimal" operating system

More useless than (Urbit)[http://urbit.org/]!

To be used for ESP8266 as a minimal functional internet OS with
"memory", see (immutable operating
system)[https://hn.algolia.com/story/7166173/an-immutable-operating-system?query=&sort=byPopularity&prefix&page=0&dateRange=all&type=all].

## Goal

Simple online virtual physical computer.

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

## features

- active messaging
- reactive, single user space, no address sharing
- no processes, or tasks, single threaded
- persistent data/program/functions once "uploaded" (TODO)
- simple evaluation mechanism
- http communication (TODO)

### Readings 

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

### Readings about x86 style OSes

- how to build and bootstrap your own os (on PC arch) (link)[http://www.brokenthorn.com/Resources/]
- http://www.menuetos.net/index.htm
- http://www.jamesmolloy.co.uk/tutorial_html/index.html
- http://www.osdever.net/FreeVGA/home.htm
- https://github.com/ReturnInfinity/BareMetal-OS
- http://wiki.osdev.org/Main_Page
- http://home.in.tum.de/~hauffa/slides.pdf
