#!/usr/bin/perl -w

# extracts errors from a header file and generates a function that maps
# the error values to names

# assumptions: 
# - 0 means no error
# - error definitions share the same prefix not used in any other identifier


if ($#ARGV != 1)
{
	print "usage: extract-error <unique-error-prefix> <function-name>\n";
	exit(1);
}

# print $ARGV[0] . "\n";

@symbols = ();

while ($line = <STDIN>) 
{
	if ($line =~ /$ARGV[0](\w*)\b/)
	{
		push(@symbols, $ARGV[0] . $1);
	}
}

if ($#symbols == -1)
{
	print STDERR "no symbol found\n";
	exit(2);
}

print "const char* $ARGV[1](int err)\n";
print "{\n";
print "\ttypedef struct { int num; const char* name; } err_entry;\n";
print "\terr_entry errors[] =\n";
print "\t{\n";
print "\t\t{ 0, \"no error\" },\n";

foreach $symbol (@symbols)
{
	print "\t\t{ $symbol, \"$symbol\" },\n";
}

print "\t};\n\n";

print "\tint i;\n";
print "\tfor(i = 0; i < sizeof(errors) / sizeof(err_entry); i++ ) {\n";
print "\t\tif( err == errors[i].num )\n";
print "\t\t\treturn errors[i].name;\n";
print "\t}\n\n";

print "\treturn \"Unknown error\";\n";
print "}\n";
