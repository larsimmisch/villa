# this code relies on the output format of cvs 1.10

$repository = `cat CVS/Repository` or die "no version found";

print "repository: " . $repository . "\n";

chomp $repository;

open STATUS, "cvs status 2>&1 |" or die "can't fork";

$status = "";

while (<STATUS>)
{
	if ($_ =~ /File:(\s*)(\S+)(\s*)Status:(\s*)(.+)/)
	{
		$status = $5;
	}
	elsif ($_ =~ /Repository revision:(\s*)(\S+)(\s*)$repository\/(.+),v/)
	{
		$file = $4;
		if ($status ne "Up-to-date")
		{
			print STDOUT $status . ": " . $file . "\n";
		}
	}
}

close STATUS or die "bad cvs status";
