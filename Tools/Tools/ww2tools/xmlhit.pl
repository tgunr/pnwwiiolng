#!/usr/bin/perl

foreach $filename (@ARGV)
{
print "$filename\n";

%damageimportant = {};

($name)=($filename=~/(.*).hit/);
open IN ,"<$name.cmp";

while(<IN>)
{
	$_ =~ s/\s+$//;
	($key, $val)=($_=~/(\S*)\s*=\s*(\S*)/);
	if($key eq "type") { $type = $val;}
	if($key=~/hitmapcomponent(.*)/)
	{
		if($type eq "airfoil"
		|| $type eq "engine"
		|| $type eq "pitot"
		|| $type eq "gear"
		|| $type eq "crew"
		|| $type eq "weaponmount")
		{
			$damageimportant{$val} = 1;
		}
	}
}
close IN;

open IN, "<$filename";

open OUT, ">$filename.xml";
print OUT "<?xml version='1.0'?>\n<!DOCTYPE state SYSTEM \"hit.dtd\">\n";
print OUT "<hitmaps>\n";

while(<IN>)
{
	$_ =~ s/\s+$//;
	if(substr($_,0,1) eq "#")
	{
		next;
	}elsif(substr($_,0,1) eq "-")
	{
		($key, $val) =($_=~/-(\S*)\s*=\s*(\S*)/);
		$val=~s/\\/\//g;
		($name)=($val=~/sounds\/(.*)\.wav/);
		if($name ne "") { $val = $name;}
		$sounds{$key} = $val;
	}else
	{
		@args = split/,/;
		if($#args > -1)
		{
			print OUT "<hitmap id=\"$args[0]\"";
			if($args[1] ne "") { print OUT " thickness=\"$args[1]\""; }
			if($args[2] ne "") { print OUT " facehardness=\"$args[2]\""; }
			if($args[3] ne "") { print OUT " avghardness=\"$args[3]\""; }
			if($args[4] ne "") { print OUT " concussionresistance=\"$args[4]\""; }
			if($args[5] ne "") { print OUT " damagethreshold=\"$args[5]\""; }
			if($damageimportant{$args[0]}==0) { print OUT " damageimportant=\"0\""; }
			print OUT "/>\n";
		}
	}
}
print OUT "<hitsound\n";
foreach $key (keys %sounds)
{
	print OUT "\t$key=\"$sounds{$key}\"\n";
}
print OUT "/>\n";
print OUT "</hitmaps>\n";
close OUT;
close IN;
}