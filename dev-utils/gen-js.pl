use strict;
use warnings;

my $downloadPageDir = shift @ARGV;
my $downloadRootDir = shift @ARGV;
if(defined($downloadPageDir))
{
	chdir $downloadPageDir;
}
if(not defined($downloadRootDir))
{
	$downloadRootDir="./types";
}

my $typeOrder = [ "images", "fon-flash", "src" ];
my $typeNames = {"src"=>"Source Code", "images"=>"Firmware Images", "fon-flash"=>"FonFlash"};
my $typeMatches={"src"=>"src", "fon-flash"=>"fon" };

my $archOrder = [ "ar71xx", "brcm47xx", "atheros", "alix" ];
my $archNames = {"brcm47xx"=>"Broadcom", "atheros"=>"Atheros 231X/5312", "ar71xx"=>"Atheros AR71XX", "alix"=>"Alix/X86" };
my $targetOrder = 
{ 
	"atheros"=>
		[
		"combined.img",
		"combined.squashfs.img",
		"root.squashfs", 
		"vmlinux.lzma", 
		"vmlinux.gz", 
		"vmlinux.elf",
		"ubnt2-pico2-squashfs.bin", 
		"ubnt2-squashfs.bin", 
		"ubnt5-squashfs.bin",
		"np25g-squashfs.bin",
		"wpe53g-squashfs.bin"

		], 
	"brcm47xx"=>
		[
		"squashfs.trx",
		"wrt54g-squashfs.bin",
		"wrtsl54gs-squashfs.bin",
		"wrt54gs-squashfs.bin",
		"wrt54gs_v4-squashfs.bin",
		"wrt54g3g-squashfs.bin",
		"wrt54g3g-em-squashfs.bin",
		"wrt54g3gv2-vf-squashfs.bin",
		"wrt150n-squashfs.bin",
		"wrt300n_v1-squashfs.bin",
		"wrt300n_v11-squashfs.bin",
		"wrt350n_v1-squashfs.bin",
		"wr850g-squashfs.bin",
		"we800g-squashfs.bin",
		"wa840g-squashfs.bin",
		"usr5461-squashfs.bin",
		"ps1208mfg-squashfs.bin",
		"wgt634u-squashfs.bin",
		"wnr834b_v2-squashfs.chk",
		"wrt610n_v1-squashfs.bin"

		],
	"ar71xx"=>
		[
		"alfa-ap96-kernel.bin",
		"alfa-ap96-rootfs-squashfs.bin",
		"alfa-ap96-squashfs-sysupgrade.bin",
		"alfa-nx-squashfs-factory.bin",
		"alfa-nx-squashfs-sysupgrade.bin",
		"hornet-ub-squashfs-factory.bin",
		"hornet-ub-squashfs-sysupgrade.bin",
		"gargoyle-pocket-router-v2-squashfs-sysupgrade.bin",
		"all0258n-kernel.bin",
		"all0258n-rootfs-squashfs.bin",
		"all0258n-squashfs-sysupgrade.bin",
		"all0305-kernel.bin",
		"all0305-rootfs-squashfs.bin",
		"all0305-squashfs-sysupgrade.bin",
		"all0315n-kernel.bin",
		"all0315n-rootfs-squashfs.bin",
		"all0315n-squashfs-sysupgrade.bin",
		"ap81-kernel.bin",
		"ap81-rootfs-squashfs.bin",
		"ap81-squashfs-sysupgrade.bin",
		"ap83-kernel.bin",
		"ap83-rootfs-squashfs.bin",
		"ap83-squashfs-sysupgrade.bin",
		"dir-600-a1-squashfs-factory.bin",
		"dir-600-a1-squashfs-sysupgrade.bin",
		"dir-601-a1-squashfs-factory.bin",
		"dir-601-a1-squashfs-sysupgrade.bin",
		"dir-615-c1-squashfs-factory.bin",
		"dir-615-c1-squashfs-sysupgrade.bin",
		"dir-615-e4-squashfs-factory.bin",
		"dir-615-e4-squashfs-sysupgrade.bin",
		"dir-825-b1-squashfs-backup-loader.bin",
		"dir-825-b1-squashfs-factory.bin",
		"dir-825-b1-squashfs-sysupgrade.bin",
		"dir-825-c1-squashfs-factory.bin",
		"dir-825-c1-squashfs-sysupgrade.bin",
		"dir-835-a1-squashfs-factory.bin",
		"dir-835-a1-squashfs-sysupgrade.bin",
		"ja76pf2-kernel.bin",
		"ja76pf2-rootfs-squashfs.bin",
		"ja76pf2-squashfs-sysupgrade.bin",
		"mzk-w04nu-squashfs-factory.bin",
		"mzk-w04nu-squashfs-sysupgrade.bin",
		"rw2458n-squashfs-factory.bin",
		"rw2458n-squashfs-sysupgrade.bin",
		"tew-637gru-squashfs-backup-loader.bin",
		"tew-637gru-squashfs-factory.bin",
		"tew-637gru-squashfs-sysupgrade.bin",
		"tew-652brp-squashfs-factory.bin",
		"tew-652brp-squashfs-sysupgrade.bin",
		"tl-mr11u-v1-squashfs-factory.bin",
		"tl-mr11u-v1-squashfs-sysupgrade.bin",
		"tl-mr3020-v1-squashfs-factory.bin",
		"tl-mr3020-v1-squashfs-sysupgrade.bin",
		"tl-mr3040-v1-squashfs-factory.bin",
		"tl-mr3040-v1-squashfs-sysupgrade.bin",
		"tl-mr3220-v1-squashfs-factory.bin",
		"tl-mr3220-v1-squashfs-sysupgrade.bin",
		"tl-mr3420-v1-squashfs-factory.bin",
		"tl-mr3420-v1-squashfs-sysupgrade.bin",
		"tl-wa701n-v1-squashfs-factory.bin",
		"tl-wa701n-v1-squashfs-sysupgrade.bin",
		"tl-wa901nd-v1-squashfs-factory.bin",
		"tl-wa901nd-v1-squashfs-sysupgrade.bin",
		"tl-wa901nd-v2-squashfs-factory.bin",
		"tl-wa901nd-v2-squashfs-sysupgrade.bin",
		"tl-wdr3600-v1-squashfs-factory.bin",
		"tl-wdr3600-v1-squashfs-sysupgrade.bin",
		"tl-wdr4300-v1-squashfs-factory.bin",
		"tl-wdr4300-v1-squashfs-sysupgrade.bin",
		"tl-wdr4310-v1-squashfs-factory.bin",
		"tl-wdr4310-v1-squashfs-sysupgrade.bin",
		"tl-wr1041n-v2-squashfs-factory.bin",
		"tl-wr1041n-v2-squashfs-sysupgrade.bin",
		"tl-wr1043nd-v1-squashfs-factory.bin",
		"tl-wr1043nd-v1-squashfs-sysupgrade.bin",
		"tl-wr1043nd-v2-squashfs-factory.bin",
		"tl-wr1043nd-v2-squashfs-sysupgrade.bin",
		"tl-wr2543-v1-squashfs-factory.bin",
		"tl-wr2543-v1-squashfs-sysupgrade.bin",
		"tl-wr703n-v1-squashfs-factory.bin",
		"tl-wr703n-v1-squashfs-sysupgrade.bin",
		"tl-wr740n-v1-squashfs-factory.bin",
		"tl-wr740n-v1-squashfs-sysupgrade.bin",
		"tl-wr740n-v3-squashfs-factory.bin",
		"tl-wr740n-v3-squashfs-sysupgrade.bin",
		"tl-wr740n-v4-squashfs-factory.bin",
		"tl-wr740n-v4-squashfs-sysupgrade.bin",
		"tl-wr741nd-v1-squashfs-factory.bin",
		"tl-wr741nd-v1-squashfs-sysupgrade.bin",
		"tl-wr741nd-v2-squashfs-factory.bin",
		"tl-wr741nd-v2-squashfs-sysupgrade.bin",
		"tl-wr741nd-v4-squashfs-factory.bin",
		"tl-wr741nd-v4-squashfs-sysupgrade.bin",
		"tl-wr743nd-v1-squashfs-factory.bin",
		"tl-wr743nd-v1-squashfs-sysupgrade.bin",
		"tl-wr841nd-v3-squashfs-factory.bin",
		"tl-wr841nd-v3-squashfs-sysupgrade.bin",
		"tl-wr841nd-v5-squashfs-factory.bin",
		"tl-wr841nd-v5-squashfs-sysupgrade.bin",
		"tl-wr841nd-v7-squashfs-factory.bin",
		"tl-wr841nd-v7-squashfs-sysupgrade.bin",
		"tl-wr841n-v8-squashfs-factory.bin",
		"tl-wr841n-v8-squashfs-sysupgrade.bin",
		"tl-wr841n-v1.5-squashfs-factory.bin",
		"tl-wr841n-v1.5-squashfs-sysupgrade.bin",
		"tl-wr842n-v1-squashfs-factory.bin",
		"tl-wr842n-v1-squashfs-sysupgrade.bin",
		"tl-wr941nd-v2-squashfs-factory.bin",
		"tl-wr941nd-v2-squashfs-sysupgrade.bin",
		"tl-wr941nd-v3-squashfs-factory.bin",
		"tl-wr941nd-v3-squashfs-sysupgrade.bin",
		"tl-wr941nd-v4-squashfs-factory.bin",
		"tl-wr941nd-v4-squashfs-sysupgrade.bin",
		"ubnt-airrouter-squashfs-factory.bin",
		"ubnt-airrouter-squashfs-sysupgrade.bin",
		"ubnt-bullet-m-squashfs-factory.bin",
		"ubnt-bullet-m-squashfs-sysupgrade.bin",
		"ubnt-ls-sr71-squashfs-factory.bin",
		"ubnt-ls-sr71-squashfs-sysupgrade.bin",
		"ubnt-nano-m-squashfs-factory.bin",
		"ubnt-nano-m-squashfs-sysupgrade.bin",
		"ubnt-rocket-m-squashfs-factory.bin",
		"ubnt-rocket-m-squashfs-sysupgrade.bin",
		"ubnt-rspro-squashfs-factory.bin",
		"ubnt-rspro-squashfs-sysupgrade.bin",
		"ubnt-rs-squashfs-factory.bin",
		"ubnt-rs-squashfs-sysupgrade.bin",
		"ubnt-unifi-squashfs-factory.bin",
		"ubnt-unifi-squashfs-sysupgrade.bin",
		"whr-g301n-squashfs-factory.bin",
		"whr-g301n-squashfs-sysupgrade.bin",
		"whr-g301n-squashfs-tftp.bin",
		"whr-hp-g300n-squashfs-factory.bin",
		"whr-hp-g300n-squashfs-sysupgrade.bin",
		"whr-hp-g300n-squashfs-tftp.bin",
		"whr-hp-gn-squashfs-factory.bin",
		"whr-hp-gn-squashfs-sysupgrade.bin",
		"whr-hp-gn-squashfs-tftp.bin",
		"wlae-ag300n-squashfs-factory.bin",
		"wlae-ag300n-squashfs-sysupgrade.bin",
		"wlae-ag300n-squashfs-tftp.bin",
		"wndr3700-squashfs-factory.img",
		"wndr3700-squashfs-factory-NA.img",
		"wndr3700-squashfs-sysupgrade.bin",
		"wndr3700v2-squashfs-factory.img",
		"wndr3700v2-squashfs-factory-NA.img",
		"wndr3700v2-squashfs-sysupgrade.bin",
		"wndr3800-squashfs-factory.img",
		"wndr3800-squashfs-sysupgrade.bin",
		"wndrmac-squashfs-factory.img",
		"wndrmac-squashfs-sysupgrade.bin",
		"wndrmacv2-squashfs-factory.img",
		"wndrmacv2-squashfs-sysupgrade.bin",
		"wrt160nl-squashfs-factory.bin",
		"wrt160nl-squashfs-sysupgrade.bin",
		"wrt400n-squashfs-factory.bin",
		"wrt400n-squashfs-sysupgrade.bin",
		"wzr-hp-ag300h-squashfs-factory.bin",
		"wzr-hp-ag300h-squashfs-sysupgrade.bin",
		"wzr-hp-ag300h-squashfs-tftp.bin",
		"wzr-hp-g300nh2-squashfs-factory.bin",
		"wzr-hp-g300nh2-squashfs-sysupgrade.bin",
		"wzr-hp-g300nh2-squashfs-tftp.bin",
		"wzr-hp-g300nh-squashfs-factory.bin",
		"wzr-hp-g300nh-squashfs-sysupgrade.bin",
		"wzr-hp-g300nh-squashfs-tftp.bin",
		"wzr-hp-g450h-squashfs-factory.bin",
		"wzr-hp-g450h-squashfs-sysupgrade.bin",
		"wzr-hp-g450h-squashfs-tftp.bin",
		"zcn-1523h-2-8-squashfs-factory.img",
		"zcn-1523h-2-8-squashfs-sysupgrade.bin",
		"zcn-1523h-5-16-squashfs-factory.img",
		"zcn-1523h-5-16-squashfs-sysupgrade.bin",
		"root.squashfs",
		"root.squashfs-4k",
		"root.squashfs-64k",
		"vmlinux.bin",
		"vmlinux.elf",
		"vmlinux.gz",
		"vmlinux.lzma"
		],
	"alix"=> 
		[ 
		"combined.jffs2.128k.img",
		]
};



open STDERR, '>/dev/null';


my $imageDir="images";


my $typeNameOrder = [];
print "var downloadData = new Array();\n";
foreach my $type (@$typeOrder)
{
	if( -d "$downloadRootDir/$type" )
	{
		my $typeVarName = $type . "Data";
		$typeVarName =~ s/[\(\)\-\.\t ]+//g; 
		print "var $typeVarName = new Array();\n";
		if($type eq "$imageDir")
		{
			my $archNameOrder = [];
			foreach my $arch (@$archOrder)
			{
				if ( -d "$downloadRootDir/$type/$arch" )
				{
					my $archName = $archNames->{$arch};
					my $archVarName = $arch . "Data";
					$archVarName =~ s/[\(\)\-\.\t ]+//g;
					print "var $archVarName = new Array();\n";

					my @fileNames = glob("$downloadRootDir/$type/$arch/*");
					my $sortedTypeFiles = sortFiles(\@fileNames);
					
					my $definedBranches = {};
					my $sortedBranches = [];
					my $definedVersions = {};
					my $sortedVersions = {};
					foreach my $f (@$sortedTypeFiles)
					{
						if($f->[1] ne "" && $f->[2] ne "" && $f->[3] ne "")
						{
							my $branch = $f->[1];
							my $version = $f->[2];
							
							
							if(not defined($definedBranches->{$branch}))
							{
								push(@$sortedBranches, $branch);
								$definedBranches->{$branch} = 1;
							}

							if(not defined($definedVersions->{$branch}->{$version}))
							{
								$definedVersions->{$branch}->{$version} = 1;
								my $vlist = defined($sortedVersions->{$branch}) ? $sortedVersions->{$branch} : [];
								push(@$vlist, $version);
								$sortedVersions->{$branch} = $vlist;
							}
						}
					}

					foreach my $branch (@$sortedBranches)
					{
						my $branchVarName =  $archVarName . "_" . $branch . "Data";
						$branchVarName =~ s/[\(\)\-\.\t ]+//g;
						print "var $branchVarName = new Array();\n";
						
						my $sortedBranchVersions = $sortedVersions->{$branch};
						foreach my $version (@$sortedBranchVersions)
						{
							my $versionVarName = $archVarName . "_" . $branch . "_" . $version . "Data";
							$versionVarName =~ s/[\(\)\-\.\t ]+//g;
							print "var $versionVarName = new Array();\n";
							
							my $fileOrder = [];
							foreach my $f (@$sortedTypeFiles)
							{
								if($f->[1] eq $branch && $f->[2] eq $version && $f->[3] ne "")
								{
									my $name = $f->[0];
									$name =~ s/^.*\///g;
									
									my $path = $f->[0];
									my $md5 = `md5 $path 2>/dev/null | awk '{ print \$4 }' ; md5sum $path 2>/dev/null | awk ' { print \$1 } '`;
									chomp $md5;
									print $versionVarName ."[\"$name\"] = [\"$path\", \"$md5\" ];\n";
									push(@$fileOrder, "\"$name\"");
								}
							}
							print $versionVarName . "[\"DATA_ORDER\"] = [ " . join(",", @$fileOrder) . " ];\n";
							print $branchVarName . "[\"$version\"] = $versionVarName;\n";
						}
						print $branchVarName . "[\"DATA_ORDER\"] = [ \"" . join("\",\"", @$sortedBranchVersions) . "\" ];\n";
						print $archVarName . "[\"$branch\"] = $branchVarName;\n";
					}
					print $archVarName . "[\"DATA_ORDER\"] = [ \"" . join("\",\"", @$sortedBranches) . "\" ];\n";
					print $typeVarName . "[\"$archName\"] = $archVarName;\n";
					push(@$archNameOrder, "\"$archName\"");
				}
			}
			print $typeVarName . "[\"DATA_ORDER\"] = [ " . join(",", @$archNameOrder) . " ];\n";
	
		}
		else
		{
			my $match = defined($typeMatches->{$type}) ? $typeMatches->{$type} : "";
			my @fileNames = glob("$downloadRootDir/$type/*$match*");
			my $sortedTypeFiles = sortFiles(\@fileNames);
			my $fileOrder = [];
			foreach my $f (@$sortedTypeFiles)
			{
				my $name = $f->[0];
				$name =~ s/^.*\///g;
				my $path = $f->[0];
				my $md5 = `md5 $path 2>/dev/null | awk '{ print \$4 }' ; md5sum $path 2>/dev/null | awk ' { print \$1 } '`;
				chomp $md5;
				print $typeVarName ."[\"$name\"] = [\"$path\", \"$md5\" ];\n";
				push(@$fileOrder, "\"$name\""); 
			}
			print $typeVarName . "[\"DATA_ORDER\"] = [ " . join(",", @$fileOrder) . " ];\n";
		}
		
	
		print "downloadData[\"" . $typeNames->{$type} . "\"] = $typeVarName;\n";
		push(@$typeNameOrder, "\"" . $typeNames->{$type} . "\"");
	}
}
print "downloadData[\"DATA_ORDER\"] = [ " . join(",", @$typeNameOrder) . " ];\n";

exit;


sub sortFiles
{
	my $fnames = shift @_;
       	my $numbered = {};
	my $others = [];
	foreach my $fname (@$fnames)
	{
		if($fname =~ /[0-9]+\.[0-9]+\.[0-9]+/)
		{
			my @versionParts = split(/\./, $&);
			my $def = [];
			if(defined($numbered->{$versionParts[0]}->{$versionParts[1]}->{$versionParts[2]}))
			{
				$def = $numbered->{$versionParts[0]}->{$versionParts[1]}->{$versionParts[2]};
			}
			push(@$def, $fname);
			$numbered->{$versionParts[0]}->{$versionParts[1]}->{$versionParts[2]} = $def;
		}
		else
		{
			push(@$others, [$fname, "", "", ""]);
		}
	}

	my $all = [];
	my $sortedNumbered = getSortedNumbered($numbered);
	push (@$all, @$sortedNumbered);
	push(@$all, @$others);
	return $all;
}

sub getSortedNumbered
{
	my $hash = shift @_;
	my $returns = shift @_;
	my $numParts = shift @_;
	if(not defined($returns))
	{
		$returns = [];
	}
	if(not defined($numParts))
	{
		$numParts = [];
	}
	my @sorted = sort {$b <=> $a} keys %$hash;
	foreach my $s (@sorted)
	{
		push(@$numParts, $s);
		my $next = $hash->{$s};
		if(scalar(@$numParts) == 3)
		{
			my $v = sortSameVersionNumber($next, $numParts->[0] . "." . $numParts->[1], join(".", @$numParts) );
			push(@$returns, @$v);
		}
		else
		{
			getSortedNumbered($next, $returns, $numParts);
		}
		pop @$numParts;
	}
	return $returns;
}

sub sortSameVersionNumber
{
	my $fileList = shift @_;
	my $branch = shift @_;
	my $version = shift @_;

	if($branch ne "")
	{
		my @splitBranch = split(/\./, $branch);
		if(scalar(@splitBranch) > 1)
		{
			if($splitBranch[1] % 2 == 0)
			{
				$branch = "$branch (stable)";
			}
			else
			{
				$branch = "$branch (experimental)";
			}
		}
	}


	#return $fileList;

	my $versionHash = {};
	my $betas = {};
	my $other = [];


	#doesn't really need to be sorted, but
	#in case anything is mixed, let's keep
	#order consistent
	my @targets = sort keys %$targetOrder;

	foreach my $f (@$fileList)
	{
		my $found = 0;
		if($f =~ /beta[0-9]+/)
		{
			my $target  = "";
			foreach my $t (@targets)
			{
				if($f =~ /$t/)
				{
					$target = $t;
				}
			}

			my $beta = $&;
			$beta =~ s/beta//g;
			my $betaFileDef =  [$f, $branch, "$version Beta $beta", $target];
			my $betaFileDefs = defined($betas->{$beta}) ? $betas->{$beta} : [];
			push(@$betaFileDefs, $betaFileDef);
			$betas->{$beta} = $betaFileDefs;
			$found = 1;
		}
		foreach my $target (@targets)
		{
			if($f =~ /$target/)
			{
				my $order = $targetOrder->{$target};
				foreach my $type (@$order)
				{
					if($f =~ /$type/)
					{
						$versionHash->{$target}->{$type} = [$f, $branch, $version, $target];
						$found = 1;
					}
				}
			}
		}
		if($found == 0)
		{
			push(@$other, [$f, $branch, $version, ""]);
		}
	}

	my $versions = [];
	foreach my $target (@targets)
	{
		my $order = $targetOrder->{$target};
		foreach my $type (@$order)
		{
			if(defined($versionHash->{$target}->{$type}))
			{
				push(@$versions, $versionHash->{$target}->{$type});
				
			}
		}
	}

	my $sortedBetas = [];
	my @sortedBetaNums = sort { $b <=>$a} keys %$betas;
	foreach my $n (@sortedBetaNums)
	{
		my $betaFileDefs = $betas->{$n};
		push(@$sortedBetas, @$betaFileDefs);
	}
	
	my $all = [];
	push(@$all, @$versions);
	push(@$all, @$other);
	push(@$all, @$sortedBetas);

	return $all;
}
