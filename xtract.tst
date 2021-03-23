=== foo: FOO\n  bar:BAR fie:FIE\nfum:FUM
foo => FOO
bar => BAR
foo: => FOO
foo:bar => BAR
foo:bar: => BAR
foo:fie: => FIE
fie => FIE
fum => FUM
foo:fum =>
fie:fum =>

### XPATH

=== <foo>FOO</foo>
foo => FOO
/foo => FOO
//foo => FOO

=== <foo>FOO<bar>BAR</bar>FIE</foo>
//bar => BAR
bar => BAR

=== <foo>F<fie/>OO<bar>BAR</bar>FIE</foo>
fie/ =>
fie =>
bar => BAR
/bar => ???

=== <xoo><foo>FOO<bar>BAR</bar>FIE</foo></xoo>
/foo =>
/xoo => <foo>FOO<bar>BAR</bar>FIE</foo>
foo/bar => BAR
xoo/foo/bar => BAR
/xoo/foo/bar => BAR
//xoo//bar => BAR

=== <xoo><abba>ABBA</abba>x sd <a>fds</a> sdf <foo>FOO<bar>BAR</bar>FIE</foo></xoo>
abba => ABBA
a => fds
abba/a =>
xoo/abba => ABBA
xoo/a => fds
xoo/abba/a =>
xoo/abba/foo =>
xoo/abba/foo/bar =>
xoo/foo/bar => BAR
xoo//bar
/xoo//bar
//xoo//bar

=== <xoo><abba>ABBA</abba><gurk>GURK</gurk><foo>FOO<bar>BAR</bar>FIE</foo></xoo>
xoo//bar => BAR
xoo/foo => <foo>FOO<bar>BAR</bar>FIE</foo>
xoo//foo => <foo>FOO<bar>BAR</bar>FIE</foo>

