#!/usr/bin/perl

#lots of variables in turret file will be moved elsewhere
%ignore=( 
	gunsightfile => 1,
	instgunsightfile => 1,
	gunsightalignmentmountobject => 1,
	instgunsightalignmentmountobject => 1,
	gunsightalignmentrange => 1,
	gunsightblocked => 1, 
	instsightblocked => 1,
	bombsight => 1,
	bombmount => 1,
	hoistturret => 1,
);

foreach $filename (@ARGV)
{
	print "$filename\n";

	open IN, "<$filename";

	open OUT, ">$filename.xml";
	print OUT "<?xml version='1.0'?>\n<!DOCTYPE turrets SYSTEM \"turret.dtd\">\n";

	print OUT "<turrets>\n";

	$turretnum = 1;

	while(<IN>)
	{
		chomp;

		($key, $val)=($_=~/(\S*)\s*=\s*(\S*)/);
		
		if($key eq "" || $val eq "") { next; }
		if(substr($key,0,1) eq "#") { next; }
		
		$val=~s/\\/\//g;
		
		if($key eq "turrettraverse"
		|| $key eq "turretelevate" 
		|| $key eq "turretgeneral" )
		{
			($name)=($val=~/(.*)\.wav/);
			if($name ne "") { $val = $name;}
			$turretsound{$key} = $val;
			next;
		}
		
		if($key eq "position")
		{
			if($turretnum>1) { print OUT "/>\n"; }

			print OUT "<turret id=\"$turretnum\"\n";
			$turretnum++;
		}else
		{
			if(!$ignore{$key})
			{
				print OUT "\t$key=\"$val\"\n";
			}
		}
		
	}
	if($turretnum>1) { print OUT "/>\n"; }

	foreach $key (keys %turretsound)
	{
		print OUT "<turretsound id=\"$key\" name=\"$turretsound{$key}\"/>\n";
	}

	print OUT "</turrets>\n";
	close IN;
	close OUT;
}