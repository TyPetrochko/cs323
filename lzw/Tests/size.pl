#!/usr/bin/perl
#
#   size.pl EXECUTABLE
#
# Use /usr/bin/size to calculate the size of EXECUTABLE
#
# The output from size is of the form
#
#   text    data     bss     dec     hex filename
#   9687     700      80   10467    28e3 encode

(@ARGV == 1)
   || die ("usage: size.pl EXECUTABLE\n");
$exec = shift;

open (SIZE, "/usr/bin/size $exec |")
   || die ("cannot /usr/bin/size $exec\n");
$line = <SIZE>;                                         # Skip header
$line = <SIZE>;

($line =~ m{^\s*(\d+\s+){3}(\d+)\s+\S+\s+(.*)$})       # Read size and name
   || die ("cannot parse: $line");
($size, $name) = ($2, $3);

($name eq $exec)
   || die ("$name ne $exec\n");

print "$size\n";
