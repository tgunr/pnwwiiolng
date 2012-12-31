#!/usr/bin/perl

foreach $filename (@ARGV)
{
print "$filename\n";

open IN, "<$filename";

@controls = ( {},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{});
@poscontrols = ( {},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{} );

while(<IN>)
{
	chomp;
	($key, $val) =($_=~/(\S*)\s*=\s*(\S*)/);

	if($key eq "" || $val eq "") { next; }

	if($key eq "position")
	{
		$position = $val;
	}elsif ($key eq "control")
	{
		$controls[$position]{$val} = 1;
	}
}	

$position = 0;
foreach $href (@controls)
{
	foreach $key (keys %$href)
	{
		$inall = 1;

		foreach $subhref (@controls)
		{
			if(%$subhref)
			{
				if(!exists $subhref->{$key})
				{
					$inall = 0;
					last;
				}
			}
		}
		
		if($inall)
		{
			$poscontrols[0]{$key} = 1;
		}else
		{
			$poscontrols[$position]{$key} = 1;
		}
	}
	
	$position++;
}

open OUT, ">$filename.xml";
print OUT "<?xml version='1.0'?>\n<!DOCTYPE controlset SYSTEM \"vehicle.dtd\">\n";
print OUT "<controlset>\n";

$position = 0;
foreach $href (@poscontrols)
{
	if(%$href)
	{
		if($position > 0)
		{
			print OUT "<position id=\"$position\">\n";
		}
		foreach $control (keys %$href)
		{
			
			if($position > 0) { print OUT "\t"; }
			print OUT "<control id=\"$control\"/>\n";
		}
		if($position > 0)
		{
			print OUT "</position>\n";
		}
	}
	$position++;
}

print OUT "</controlset>\n";

close IN;
close OUT;
}