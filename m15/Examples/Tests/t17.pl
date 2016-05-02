#!/usr/bin/perl
#
#    t17.pl [length]
#
#  Linear time/space: Very long macro NAME

$length = (defined $ARGV[0]) ? $ARGV[0] : 1;

$longName = "a" x $length;
print "\\def{$longName}{b}\\$longName\{c}\n";
