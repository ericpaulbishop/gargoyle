#!/usr/bin/perl

use strict;
use warnings;


my $dir = shift @ARGV;

my @dirFiles = glob("$dir/*");
foreach my $df (@dirFiles)
{
	linux_newline_format($df);
}


sub linux_newline_format
{
	my $file = shift @_;
	if( -d "$file")
	{
		my @subdirFiles = glob("$file/*");
		foreach my $sdf (@subdirFiles)
		{
			linux_newline_format($sdf);
		}
	}
	else
	{
		print "$file\n";
		my $tmpFile = "$file.tmp";
		my $nextTmpFile = $tmpFile;
		my $tmpFileCount = 1;
		while( -f "$nextTmpFile" )
		{
			$nextTmpFile = "$tmpFile.$tmpFileCount";
			$tmpFileCount = $tmpFileCount+ 1;
		}
		$tmpFile = $nextTmpFile;


		open IN, "<$file";
		open OUT, ">$tmpFile";
		while (my $line = <IN>)
		{
			chomp $line;
			$line =~ s/[\r\n]//g;
			print OUT "$line\n";
		}
		close IN;
		close OUT;

		system("mv \"$tmpFile\" \"$file\"");


	}
}

exit;

