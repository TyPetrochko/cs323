#!/usr/bin/perl
#
#    t16.pl [length]
#
#  Linear time/space: Very long input with no macros

$length = (defined $ARGV[0]) ? $ARGV[0] : 1;

$longInput = "a" x $length;
print "$longInput\n";
