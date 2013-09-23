#!/usr/bin/perl

# This script takes targets dir as argument, then enables 
# all kernel packages that are configured as a module or built-in in ANY
# subtarget as a module in ALL subtargets where they are not currently selected at all


my $targetsDir = shift @ARGV;

my @arches = glob("$targetsDir/*");

foreach my $arch (@arches)
{
	my @profiles = glob("$arch/profiles/*");
	if(scalar(@profiles) > 1)
	{
		my $kmodPackages = {};
		$profileKmodPackages = {};
		foreach $profile (@profiles)
		{
			my $config = "$profile/config";
			if ( -e "$config")
			{
				$profileKmodPackages->{$profile} = {};
				open IN, "<$config";
				while (my $line = <IN>)
				{
					chomp $line;
					if($line =~ /CONFIG_PACKAGE_kmod\-/ && $line !~ /is not set/ )
					{
						my $package = $line;
						$package =~ s/^.*CONFIG_PACKAGE_//g;
						$package =~ s/[=\t ]+.*$//g;
						$kmodPackages->{$package} = 1;
						$profileKmodPackages->{$profile}->{$package} = 1;
						print "$package\n";
					}
				}
				close IN;
			}
		}
		foreach $profile (@profiles)
		{
			my $config = "$profile/config";
			if ( -e "$config")
			{
				my $existingKmods =  $profileKmodPackages->{$profile};
				open IN, "<$config";
				open OUT, ">$config.tmp";
				while (my $line = <IN>)
				{
					chomp $line;
					
					my $newLine = $line;
					if($line =~ /CONFIG_PACKAGE_kmod\-/ && $line =~ /is not set/ )
					{
						my $package = $line;
						$package =~ s/^.*CONFIG_PACKAGE_//g;
						$package =~ s/[=\t ]+.*$//g;
						print "not set package = $package\n";

						if(defined($kmodPackages->{$package}))
						{
							$newLine="CONFIG_PACKAGE_${package}=m";
							print "$newLine\n";
							$existingKmods->{$package}=1;
						}
					}
					print OUT "$newLine\n";
				}
				close IN;

				foreach my $kmod (keys %$kmodPackages)
				{
					if(not defined($existingKmods->{$kmod}))
					{
						print OUT "CONFIG_PACKAGE_${kmod}=m\n";
					}
				}
				close OUT;
				rename("$config.tmp", "$config");
			}
		}
	}
}

