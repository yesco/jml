# a very simple implementation of example: jml in perl

# TEST: (echo "[macro dec \$A][- \$A 1][/macro][* [x] 77 [+ 3 4 [* 7 2] [map inc [iota 1 10]]]] DEC=[dec 7]" ; echo "[+ 10 20 30] ARGS=[args 1 2  3]") | perl jml.pl

# set to 1 to get trace of substitutions
$trace = 1;

my %funs = ();

sub fun {
    my($name, $fun) = @_;
    if (ref($fun) eq "CODE") {
        $funs{$name} = $fun;
    } else {
        # if it's a string, create a function using closure
        $funs{$name} = sub {
            my @args = @_;
            local($a, $b, $c, $d, $e, $f, $g) = @args;
            return eval $fun;
        };
    }
}

sub macro {
    local($name, $vars, $expr) = @_;
    fun($name, eval "sub { local($vars) = \@_; return \"$expr\"; };");
    return "";
}

# define function as simple string expression or pass sub
fun('+', sub { my $r = 0; $r += $_ for @_; return $r; });
fun('-', '$a-$b');
fun('*', sub { my $r = 1; $s *= $_ for @_; return $r; });
fun('/', '$a/$b');
fun('inc', '$a+1');
fun('iota', 'join(" ", ($a..$b))');
fun('args', 'join(" ", @args)');
fun('map', 'join(" ", map { "[$a $_]" } @args[1..$#args])');

# read-eval loop
print jml($_) for <>;

sub jml {
    local($line) = @_;
    while ($line =~ s/\[macro (\S+)\s*([^\[\]]*)\](.*?)\[\/macro\]/macro($1, $2, $3)/ge) {}
    print STDERR ">>>$line" if $trace;
    while ($line =~ s/\[(\S+)\s*([^\[\]]*?)\]/&replace($1, $2)/ge) {
        print STDERR ">>>$line" if $trace
    }
    return $line;
}

sub replace {
    local($name, $args) = @_;
    return "($name $args)" if ($name.$args) =~ /\(%FAIL:/;
    my $fun = $funs{$name};
    if (ref($fun) eq "CODE") {
        return ''. $fun->(split(/\s+/, $args));
    } else {
        return "(%FAIL:@_)";
    }
}


