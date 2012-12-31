#!/usr/bin/perl

foreach $filename (@ARGV)
{
print "$filename\n";

open IN, "<$filename";

open OUT, ">$filename.xml";
print OUT "<?xml version='1.0'?>\n<!DOCTYPE state SYSTEM \"state.dtd\">\n";
print OUT "<states>\n";

while(<IN>)
{
	chomp;
	if(substr($_,0,1) eq "#") { next; }

	@args=split;
	($state)=($args[0]=~/(.*)state/);
	if($state eq "") { $state = $args[0]; }
	if($state eq "") { next; }
		
	if($state eq "visdamage")
	{
		print OUT "<visdamage id=\"$args[1]\"\n";
		print OUT "/>\n";
	}elsif($state eq "dof")
	{
		if($args[3] eq "gear")
		{
			print OUT "<$state id=\"$args[3]\"\n";
			print OUT "\tanimtime=\"$args[4]\"\n";
		}elsif($args[3] eq "bombbaydoor")
		{
			print OUT "<$state id=\"$args[3]\"\n\tanimtime=\"$args[4]\"\n";
		}else
		{
			print OUT "<$state id=\"$args[3]$args[4]\"\n";
			
			if($args[3] eq "prop")
			{
				print OUT "\tdiscrpm=\"$args[5]\"\n";
			}else
			{
				print OUT "\tanimtime=\"$args[5]\"\n";
			}
		}
		print OUT "\tmulticrewpos=\"$args[1]\"\n";
		print OUT "/>\n";
	}elsif($state eq "smoke")
	{
		if(($exhaustidx)=($args[2]=~/exhaust(.*)/))
		{
			($lr)=($args[3]=~/.*(.+)/);
			print OUT "<$state id=\"exhaust$exhaustidx$lr\"\n";
		}else
		{
			print OUT "<$state id=\"$args[2]\"\n";
		}
		print OUT "\tobjectname=\"$args[3]\"\n";
		print OUT "\tmulticrewpos=\"$args[1]\"\n";
		print OUT "/>\n";
	
	}elsif($state eq "sound")
	{
		print OUT "<$state id=\"$args[2]\"\n";
		print OUT "\tmulticrewpos=\"$args[1]\"\n";
		print OUT "/>\n";
	}elsif($state eq "effect")
	{
		print OUT "<$state id=\"$args[2]\"\n";
		print OUT "\tmulticrewpos=\"$args[1]\"\n";
		print OUT "/>\n";
	}
}
print OUT "</states>\n";
close OUT;
close IN;
}