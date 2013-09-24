#!/usr/bin/perl

# This script takes targets dir as argument, then enables 
# all packages that are configured as a module or built-in in ANY
# subtarget as a module in ALL subtargets where they are not currently selected at all


my $targetsDir = shift @ARGV;

my @arches = glob("$targetsDir/*");

foreach my $arch (@arches)
{
	my @profiles = glob("$arch/profiles/*");
	if(scalar(@profiles) > 1)
	{
		my $packages = {};
		$profilePackages = {};
		foreach $profile (@profiles)
		{
			my $config = "$profile/config";
			if ( -e "$config")
			{
				$profilePackages->{$profile} = {};
				open IN, "<$config";
				while (my $line = <IN>)
				{
					chomp $line;
					if($line =~ /CONFIG_PACKAGE_/ && $line !~ /is not set/ )
					{
						my $package = $line;
						$package =~ s/^.*CONFIG_PACKAGE_//g;
						$package =~ s/[=\t ]+.*$//g;
						$packages->{$package} = 1;
						$profilePackages->{$profile}->{$package} = 1;
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
				my $existingPackages =  $profilePackages->{$profile};
				open IN, "<$config";
				open OUT, ">$config.tmp";
				while (my $line = <IN>)
				{
					chomp $line;
					
					my $newLine = $line;
					if($line =~ /CONFIG_/ && $line =~ /is not set/ )
					{
						my $package = $line;
						$package =~ s/^.*CONFIG_PACKAGE_//g;
						$package =~ s/[=\t ]+.*$//g;
						print "not set package = $package\n";

						if(defined($packages->{$package}))
						{
							$newLine="CONFIG_PACKAGE_${package}=m";
							print "$newLine\n";
							$existingPackages->{$package}=1;
						}
					}
					print OUT "$newLine\n";
				}
				close IN;

				foreach my $p (keys %$packages)
				{
					if(not defined($existingPackages->{$p}))
					{
						print OUT "CONFIG_${p}=m\n";
					}
				}
				close OUT;
				rename("$config.tmp", "$config");
			}
		}
	}
}

