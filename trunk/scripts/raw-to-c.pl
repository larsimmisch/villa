#!/usr/bin/perl -w

if ($#ARGV != 2)
{
	print "usage: raw-to-c <infile> <outfile> <name>\n";
	print "usage: raw-to-c reads the data from <filename> and creates\n";
	print "a const char[] with <name> in <outfile>\n";
	exit(1);
}

open INPUT, $ARGV[0] or die;
binmode INPUT;
open OUTPUT, ">$ARGV[1]" or die;

print OUTPUT "static const unsigned char $ARGV[2]\[\] = {\n";

my $buffer;
my $c;

while (my $size = read INPUT, $buffer, 20)
{
	print OUTPUT "\t";

	for (my $i = 0; $i < $size; $i++)
	{
		$c = vec $buffer, $i, 8;
		printf OUTPUT "0x%02x, ", $c;
	}
	print OUTPUT "\n";
}

print OUTPUT "};\n"