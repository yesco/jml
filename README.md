# jml - a useful web/cloud minimal unikernel distributed operating system

TODO: rename to [MagniOS](https://en.wikipedia.org/wiki/M%C3%B3%C3%B0i_and_Magni)

## Goal

Simple online-programmable virtual physical computer with built in
persistent "memory" extending the IoT to simple internet/cloud
computers, that are disposable scalable online reconfigurable minimal
processing instances. This is an instance of a "single (minimalish)
computer".

But as we know the future is distributed and we need to provide
infrastructure "above a single computer". This system provides such
services implemented utilizing the said single computers.

## Woa!?

So crassly seen it's a html-generating programmable virtual computer
with high-level lambda style "instructions". As we know, lambda
calculus is "described" by subsitution, here we take the approach that
actual in-place substitution constitutes a viable computer. We provide
a minimal of basic services without getting stuck in the trap of
generality of an operating system. All the services/functions are
"not" mapped to or dependent on a UNIX computer as such. Therefore, it
can be hosted on containerized physical hardware SOAC (System On A
Chip) like the ESP-8266.

### Services provided

(Things in parenthesis in progress..)

- Lambda style computational engine
- (Reliable) Messaging to remote entity
- Dispatch of incoming messages to user define API
- Persistent local data store and extensible (pluggable) program
- Encryption secured remote execution, and program upgrade
- (Transferable state, movability)
- CHORD-style distributed storage and addressing
- (distributed service directory)
- (distributed files and directories)

Current limitations:
- messaging currently limited to "REST" calls, could/should use UDP for small messages, otherwise TCP/IP
- messaging not reliable/transactionable
- persistence not atomic per triggering event/message

It's not clear what would be missing as a minimized viable portable
computing instance. If it's hardware connected to physical sensors
measurement naturally it would be limited to such capable instances,
often even to specific locations. Other functionality is portable, and
transferable.

## First Principles

This is a [First Principle's](https://en.wikipedia.org/wiki/First_principle) project.
It means it starts from "scratch" with minimal requirements. Elon Musk
uses this as his [innovation
principle](http://99u.com/workbook/20482/how-elon-musk-thinks-the-first-principles-method).

For example: "In physics, a calculation is said to be from first
principles, or ab initio, if it starts directly at the level of
established laws of physics and does not make assumptions such as
empirical model and fitting parameters." - wikipedia

## Requirements

C (gcc) compiler, and simple unix style standard library calls.
For "security" no external libraries are used.

## Why

The idea of a minimal OS is coming up again, maybe recently more (now
is 2016) than the last 30 years. We have docker, and various
virtualization environments. It's funny to simulate an actual once
existing hardware computer - mostly just a IBM PC compatible. However,
there are other approaches, more minimal which are single language
based. These are sometimes called a library OS implementations. For
example RTOS for ESP8266
[esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos) can be
seen as such. It provides a small implementation of most of the
libraries and system calls you'd expect on a POSIX/linux style
computer. It is limited to a single process, so no fork/popen or
system cals. It does provide it's own threading/tasks and
semaphore/messaging system. However, it doesn't provide infrastructure
to think "above a single computer".

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
- NONMINAL: what's the minimal set of functions needed?
- connectivity, receive requests/send requests (messaging)
- logical + physical addressing something like [Kademlia](https://en.wikipedia.org/wiki/Kademlia)
- callback verification [Magic cookie](https://en.wikipedia.org/wiki/Magic_cookie)
- https://en.wikipedia.org/wiki/Merkle_tree
- cheap hashes https://en.wikipedia.org/wiki/Tiger_(cryptography)

## Extentions?

It's not mean to be extended on the C-level. However, some way of
accessing hardware/sensors is needed.  Possibly, the most portable way
to approach this is to provide the service as a minimal
webrequest/mqtt interface. This would allow the sensors to be directly
accessible, if secure, on external interfaces too,
transparently. Indistinguishable from other mqtt/webservices devices.
The "strict" goal is to keep all the code/native functions, *exactly*
the *same* on all devices. In princple, after a while, we should be
able to freeze them, and only work on extension on the soft-layer.

## How to run

### command line interactive

    ./run

### batch

This allows a single command, or several to be run and output captured.

    echo "[concat a b c]" | ./run 2>/dev/null

### web

To run it as a webserve on port 1111, or as given use the command below.
Connect to it as [localhost:1111/](localhost:1111/).

    ./run -w [PORT]

### simulated group of servers

    ./start-servers

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

### options for performance/info

    -q       == quiet mode, only writes %%error messages
    -w       == normal mode, start webserver
    -v       == log timing and reductions info
    -v -v    == log reallocs
    -v -v -v == log allocs too

# Language

The language is a bastard variant of lambda calculus where the
evalution is eager, and evaluation is performed by plain textual
substitution. There are no variables as such, no stack, no closure, no
circular data structures, no data structures. Semantics of our
langauges is specified by legal substitutions. We find any "innermost"
expression [FUN ARGS...] where FUN and ARGS do not contain any other
expression (i.e. '[' or ']'). These are directly substitutable. Any
number of such expressions may be substituted in any order as
determined by the evaluator. Keep it functional, and it's safe! For
achieving specific order of substitutions, create a "data dependency"
where the dependent calculations wraps the calculation that are needed
to be performed before the dependent calculation.

## simple string based evaluation

    foo => foo
    foo [+ 3 4] bar => foo 7 bar
    foo [upper bar] fie => foo BAR fie
    [[concat up per] fie] = FIE

What is an if-statement, or any choice of execution path, if not just
that: a choice of execution path!  In this language, we provide an
'if' that will return which named function to invoke on the rest of
the data;

    [if 0 1 2] => 2
    [if 1 1 2] => 1
    [if [< 3 4] smaller bigger] => smaller

From this it can be seen that it is NOT a special form or have any
specific rules, it only provides a choice of functions to invoke 
on given arguments.

For example, to implement fac recursively:

    [macro fac $n][* $n [[if [<= $n 1] ignore fac] [- $n 1]]][/macro]

    > [fac 6]
    >>>[fac 6]<<<
    >>>[* 6 [[if [<= 6 1] ignore fac] [- 6 1]]]<<<
    >>>[* 6 [[if 0 ignore fac] 5]]<<<
    >>>[* 6 [fac 5]]<<<
    >>>[* 6 [* 5 [[if [<= 5 1] ignore fac] [- 5 1]]]]<<<
    >>>[* 6 [* 5 [[if 0 ignore fac] 4]]]<<<
    >>>[* 6 [* 5 [fac 4]]]<<<
    >>>[* 6 [* 5 [* 4 [[if [<= 4 1] ignore fac] [- 4 1]]]]]<<<
    >>>[* 6 [* 5 [* 4 [[if 0 ignore fac] 3]]]]<<<
    >>>[* 6 [* 5 [* 4 [fac 3]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 [[if [<= 3 1] ignore fac] [- 3 1]]]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 [[if 0 ignore fac] 2]]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 [fac 2]]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 [* 2 [[if [<= 2 1] ignore fac] [- 2 1]]]]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 [* 2 [[if 0 ignore fac] 1]]]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 [* 2 [fac 1]]]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 [* 2 [* 1 [[if [<= 1 1] ignore fac] [- 1 1]]]]]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 [* 2 [* 1 [[if 1 ignore fac] 0]]]]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 [* 2 [* 1 [ignore 0]]]]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 [* 2 [* 1 ]]]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 [* 2 1]]]]]<<<
    >>>[* 6 [* 5 [* 4 [* 3 2]]]]<<<
    >>>[* 6 [* 5 [* 4 6]]]<<<
    >>>[* 6 [* 5 24]]<<<
    >>>[* 6 120]<<<
    >>>720<<<
    720

Obviously a more efficient implementation is:

    [macro fac $n][* [iota 1 $n]][/macro]

    > [fac 6]
    >>>[fac 6]<<<
    >>>[* [iota 1 6]]<<<
    >>>[* 1 2 3 4 5 6 ]<<<
    >>>720<<<
    720

It can be used and misunderstood easily, the world be dammned!

    [macro foo]11 22[/macro]
    [macro bar]2 1[/macro]

    [if 0 [foo] [bar]] => 22

    ???

    [if 0 [foo] [bar]] =>
    [if 0 11 22 [bar]] =>
    [if 0 11 22 2 1]] =>
    22 !!! second argument returned

How to do IFFFF THEN?

    [[if 0 foo bar]] => 2 1
    [[if 1 foo bar]] => 11 22

Now, gettin into the gritty, this can be used to implement choice/switch/case-statements!

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

Goto:

    http:localhost:1111/help

## super eager evaluation

The interpreter is very simple: it just eagerly evaluates any inner
[fun params...] expression where fun and params themselves have no
function calls. I.e., the innermost [ ] expressions are replaced
successively, the result is achieved by replacing the expression with
the body where the formal parameter names have been substituted by the
actual values. Parameters are space delimited.

## efficiency

Bah... computers are fast! Seriously, we're generating HTML and serving
it on the net, so what's fast anyway?

## features

- active messaging
- reactive, single user space, no address sharing
- no processes, or tasks, single threaded
- persistent data/program/functions once "uploaded"
- simple evaluation mechanism
- http communication (TODO)

## built-in functions

Math
- % inc dec > < >= <= = !=
- [+ 1 2 3 4] => 10
- [* 1 2 3 4] => 24
- [iota 1 3] => 1 2 3
- [iota 0 10 2] 

Logic
- [and 1 1 ...] => 1
- [and 1 0 ...] => 0
- [not 0] => 1
- [not 7] => 0
- [xor 0 0] = [xor 1 1] => 0
- [xor 0 1] = [xor 1 0] => 1

    val  X: empty  has  is  not number alpha
    ========================================
    [X   ]    1     0    0    1    0     0
    [X  0]    0     1    0    1    1     0 
    [X  1]    0     1    1    0    1     0
    [X  a]    0     1    1    0    0     1

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

- [length] => 0
- [length 1 2 3] => 3
- [length 11 22 33 44] => 4

- [bytes] => 0
- [bytes 1 2 3] => 5
- [bytes 1 2 3 ] => 6
- [bytes  1 2 3 ] => 7

Lists
- [ignore X] =>
- [identity X ...] => X ...

- [map F a b c ...] => [F a] [F b] [F c] [map F ...]
- [filter P a b c ...] => [[if [P a] identity ignore] a] ...
- [filter-do P F a b c ...] => [[if [P a] F ignore] a] ...

- [nth 2 11 22 33] => 22
- [first A B] => A
- [second A B C] => B
- [third A B C D] => C
- [rest A B C D] => B C D

Strings
- [after X abcXzy] => zy
- [before X abcXzy] = abc
- [prefix abcdef abc abcd ab] => ab
- [split a aAaBBaAa] => A BB A
- [split-do inc a1a22a3a] => 2 23 4
- [xml name ksajf; sadflk dsaflk <name c='foo'>FISH</name> sdfl sadf asdfdsa] => FISH

- [substr FIRST LEN abcXzy] get substrings out
- [substr 0 3 abcXzy] => abc, this is essentially "left"
- [substr 3 2 abcXzy] => Xz, this is essentially "mid"
- [substr 4 1 abcXzy] => z
- [substr -2 -2 abcXzy] => zy get the last N character, this is "right"

- [concat A B C D ...] => ABCD...
- [concat A\ B C D ...] => A BCD...
- [concat [concat a\ b c]] => A BCD...

- [match a(b*)(cd*)e(.*)f abbbcexxxxxfff] =>  bbb c xxxxx
- [match-do F a(b*)(cd*)e(.*)f abbbcexxxxxfff] => [F bbb c xxxxx]
- [match-do F <(a*)> aa<aaa><aaaa>aa<a>fish<a>x] => [F aaa] [F aaaa] [F a] [f a]
- [subst-do F <(a*)> aa<aaa><aaaa>aa<a>fish<a>x] => aa[F aaa][F aaaa]aa[F a]fish[f a]x

WEB, decode URL
- [decode foo+bar%2b%25] => foo bar+%
- [wget URL] => BODY (quoted for safety)
- [eval/FUN1/Fun2 ...] - see below!

Storage/persistent/database
- [data firstname Peter] .. [data-firstname] => Peter
- [datas] =? firstname ...
- [funcs] => user defined func names
- [funcs prefix] => user defined func names starting with prefix
- [funcs start end] => user defined func names in [start, end[
- [fargs macroname] -> $a $b @foo
- [fbody macroname] -> foo $a fie $b fum: @foo

Failure
- [xyz sadfasdf] => [FAIL xyz sadfasdf]
- [FAIL $id @params] => %(FAIL:xyz sadfasdf) - you can override this!

### Content Addressable Network

We also add the capability of (CAN)[https://en.wikipedia.org/wiki/Content_addressable_network].

For now we just implement Content Hashing by using the encrypt
function with a fixed non-secret key.  It may not qualify as
cryptographically secure hashing function, but serves the purpose of a
decent hash function, as it's already have it...

TODO: ...

#### Node Joining
A joining node must:
- Find a node already in the overlay network
- Identify a zone that can be split
- Update the routing tables of nodes neighbouring the newly split node

#### Node Departing
To handle node departing:
- Identify a node departing
- have node's zone merged or taken over by a neighbouring node
- update the routing tables across the network

heartbeat to neighbours

- TODO: Consider (implementing) mDNS
- TODO: Consider if we should be using https://en.wikipedia.org/wiki/Kademlia
- p2p filesystem w some redundancy - [PAST](https://en.wikipedia.org/wiki/Pastry_(DHT))
- p2p pubsub - [SCRIBE](https://en.wikipedia.org/wiki/Pastry_(DHT))
- http://stackoverflow.com/questions/3076222/top-hashing-and-encryption-algorithms

#### hotspots

TODO: maybe we're doing (implementing) this???

When searching for NAMED data, we should search for:

    [ NAME-HASH    '/' YOUR-ID-HASH '/' ]
    [ CHUNK-HASH   '/' YOUR-ID-HASH '/' ]

For example when a file name is hashed for filesystem implementations there may be many
hosters/locations storing the same file and registering the same file, in order not to
overload a single "bucket" we need to be able to split on a "longer" key so, key for
insertions is:

    [ NAME-HASH    '/' HOSTER-ID-HASH '/' LIST-HASH ':name' ]  => NAME
    [ LIST-HASH    '/' HOSTER-ID-HASH ':file']                 => LIST of (offset, CHUNK)
    [ CHUNK-HASH   '/' HOSTER-ID-HASH ':chunk']                => CONTENT

This way the normal splitting would be able to handle hotspots by just splitting the range.
When searching you identify yourself and will be routed to the bucket for the same CONTENT-HASH
that is closest to your ID.

Example:

file content hash
    > echo "[content-hash <data...tatatatatat>]" | ./run -q
    0A543F0365179C05

    > echo "[content-hash <data...other>]" | ./run -q
    C0458858B721576B

file name component "StarWarsIV" probably also will have "star" "wars" "iv"...
    > echo "[content-hash StarWarsIV]" | ./run -q -t
    A2065D7267A72D3D

    star => 015BA7A2250FA4D1
    wars => 6181303D2E3EEB6C
    iv   => 63AB646145DFFEA9

another file is "star trek"

    star => 015BA7A2250FA4D1
    trek => EC8DA41B66A78863

this is your node ID:

    > uuid
    5aaf5bb2-aca3-11e6-b0d5-cb6044163794
    > echo "[content-hash 5aaf5bb2-aca3-11e6-b0d5-cb6044163794]" | ./run -q -t
    DA623E175D65C3C0

we have 2 more nodes storing this file
    
    64CDE172E66BA52D
    FD017067ECF5E177

the second file is stored by you and another person:

    4B3722E05881C728

3 nodes have the file thus we will store 3 pointers in the extended key
here is the full table

    015BA7A2250FA4D1/4B3722E05881C728/C0458858B721576B => star
    015BA7A2250FA4D1/64CDE172E66BA52D/0A543F0365179C05 => star
    015BA7A2250FA4D1/DA623E175D65C3C0/0A543F0365179C05 => star
    015BA7A2250FA4D1/DA623E175D65C3C0/C0458858B721576B => star
    015BA7A2250FA4D1/FD017067ECF5E177/0A543F0365179C05 => star

    6181303D2E3EEB6C/64CDE172E66BA52D/0A543F0365179C05 => wars
    6181303D2E3EEB6C/DA623E175D65C3C0/0A543F0365179C05 => wars
    6181303D2E3EEB6C/FD017067ECF5E177/0A543F0365179C05 => wars

    A2065D7267A72D3D/64CDE172E66BA52D/0A543F0365179C05 => StarWarsIV
    A2065D7267A72D3D/DA623E175D65C3C0/0A543F0365179C05 => StarWarsIV
    A2065D7267A72D3D/FD017067ECF5E177/0A543F0365179C05 => StarWarsIV

    EC8DA41B66A78863/4B3722E05881C728/C0458858B721576B => trek
    EC8DA41B66A78863/DA623E175D65C3C0/C0458858B721576B => trek

#### example commands

    echo "[wget [route-data [content-hash Hello. This is a message!]]/id]" | ./run -q -t

This hashes by content, finds the route, connects to that server, retrieves /id URL
that should contain its hash.

#### We need a distributed filesystem

see above?

Two variants
- CA, Content Addressable, hash the content, store on the node as owner, and "k nearest".
- by URL name, like PAST: hash by URL to find (date,size,CA-hashes)

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

In order to not allow injection, certain "web" characters should be
quoted on "input" from the web or external sources. Examples include:
&lt; &gt; \[ \] &amp; ' "

### eval/fun1/fun2/fun3...

Having an eval is flexible, however, a big security risk. We take the
middle-ground and provide a "sandboxed" eval. When eval is invoked it
also requires an enumeration of all functions that are allowed to be
called.  Essentially this defines an API. For example to define a
calculator with only plus minus times divide you specify
[eval/plus/minus/times/divide ...]  Eval will change then change each
occurance of {FUN...} to [FUN...] if fun is listed. If {} doesn't
balance, empty string is returned, same if unlisted FUN is
mentioned. This doesn't limit further eval/substituations.

This, for example, can be used to implement a forward request in
CHORD/DHT by having the server return updates as well as a new
request in case the route is wrong. If the routing is right
the contacted server would return it's own ID. Otherwise it
returns a "textual-continuation" (program with data filled in),
which coincidentially is exactly what a jml-program is!

    [route-resolve $HASH]

If this doesn't resolve to the server you're at, it'll wget call that server and
it will either confirm, or return "forwarding instructions". Essentially,
some updates (route-add) and then a new route-resolve that depends on those updates.

    {route-resolve $HASH {route-add 0375F9AB200504E9 2016-... http:...} {route-add DB348B3005A37278 2016-... http:...}}

These are performed at/by the originating server, which will use:

    [eval/route-resolve/route-add ...]

to evaluate, if it again doesn't resolve locally, we'll get another
forward and the new remote server will conform that it is the
receipient by returning it's ID:

    $HOST_ID
    
#### Unicode

TODO: Haha, come again? Don't you know the world consists of bits and bytes?

Seriously, the web is multilingular, and I travel in china, use swedish so UTF-8 is a reasonable requirement.

[UTF-8](https://en.wikipedia.org/wiki/UTF-8):

"And ASCII bytes do not occur when encoding non-ASCII code points into UTF-8, making UTF-8 safe to use within most programming and document languages that interpret certain ASCII characters in a special way, e.g. as end of string."

### Security/Encryption/(TEA)[https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm]

- [encrypt FOOBAR] => {39CE0CD92EEBACC8}
- [decrypt {39CE0CD92EEBACC8}] => FOOBAR
- [encrypt-eval {+ 3 4}] => {D9403DC570A74AF1}
- [decrypt {D9403DC570A74AF1}] => {+ 3 4}
- [decrypt-eval {D9403DC570A74AF1}] => [+ 3 4] => 7

#### using custom keys

The default key is "1234123412341234", maximum lenght is 16, ascii, no
"/" or " " character allowed.  The key is part of the "function" name
(funny hack) thus the name to use is [encrypt/MYSECURITYKEY data to
encrypt], similarly for decrypt/MYSECURITYKEY,
encrypt-eval/decrypt-eval...

*Note* on "security", it's just XXTEA which can only provide nominal
security in access. They keys aren't protected and if one has access
to the physical device would be able to extract the key. However, each
instance should have it's own key and the person communicating with it
need it too.  This means; It's protected against middle-man
attack. Each unikernel/device should have it's own key. It may be
possible to use the key to only allow the owner of that key to modify
and run any code. Others would only be allowed to interact using the
public defined macro functions, like [macro /foo]...

Encrypted data becomes HEX coded, to keep it ascii, thus it will at
use at least the double amount of bytes.

TODO: enable setting/changing keys

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
