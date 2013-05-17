#!/usr/bin/perl

use strict; 
use warnings;

my $file = shift @ARGV;

my $tmpFile = "/tmp/adj.tmp.tmp";

open OUT, ">$tmpFile";

open IN, "<$file";
while (my $line = <IN>)
{
	chomp $line;
	if(	
		#$line =~ /# CONFIG_PACKAGE_kmod.* is not set/ ||
	       	$line =~ /# CONFIG_PACKAGE_plugin-gargoyle-logread.* is not set/ ||
		$line =~ /# CONFIG_PACKAGE_plugin-gargoyle-webshell.* is not set/ || 
		$line =~ /# CONFIG_PACKAGE_plugin-gargoyle-wifi-schedule.* is not set/
	)
	{
		$line =~ s/# CONFIG_PACKAGE/CONFIG_PACKAGE/g;
		$line =~ s/ is not set/=m/g;
	}
	print OUT "$line\n";
}
close IN;
close OUT;

rename("$tmpFile", "$file");


