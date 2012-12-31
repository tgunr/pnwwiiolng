#!/usr/bin/perl

foreach $filename (@ARGV)
{
($name)=($filename=~/.*\/(.*)\.veh/);
print "$filename\n";

open IN, "<$filename";

open OUT, ">$filename.xml";
print OUT "<?xml version='1.0'?>\n<!DOCTYPE include SYSTEM \"include.dtd\">\n";
print OUT "<include>\n";
print OUT "<include file=\"$name.veh\"/>\n";

while(<IN>)
{
	chomp;
	($key, $val)=($_=~/(\S*)\s*=\s*(\S*)/);
	
	if($key eq "" || $val eq "") { next; }
	if(substr($key,0,1) eq "#") { next; }
	
	if($key eq "componentfile" 
	|| $key eq "enginefile" 
	|| $key eq "positionfile")
	{
		print OUT "<include file=\"$val.xml\"/>\n";
	}
}
print OUT "</include>\n";
close OUT;
close IN;
}