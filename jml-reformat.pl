#!/bin/env perl
$last = "";
while(<>) {
    chop;
    if ($_ =~ /^\s/) {
	$last.= $_;
    } else {
	print "$last\n";
	$last= $_;
    }
}
print "$last\n";

