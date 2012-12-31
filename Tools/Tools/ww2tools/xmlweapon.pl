#!/usr/bin/perl

open AMMO, "<$ARGV[0]";

($path)=$ARGV[0]=~/(.*)\/+.*/;

while(<AMMO>)
{
chomp;
if(substr($_,0,1) eq "#") { next; }
if($_ eq "") { next; }

$ammo = $_;
$filename = "$path/$ammo";
print "$filename\n";

open IN, "<$filename";

open OUT, ">$filename.xml";
print OUT "<?xml version='1.0'?>\n<!DOCTYPE weapon SYSTEM \"weapon.dtd\">\n";

print OUT "<weapon id=\"$ammo\"\n";

while(<IN>)
{
	chomp;
	($key, $val)=($_=~/(\S*)\s*=\s*(\S*)/);
	
	if($key eq "" || $val eq "") { next; }
	if(substr($key,0,1) eq "#") { next; }
	
	print OUT "\t$key=\"$val\"\n";
}
print OUT "/>\n";
close IN;
close OUT;
}

close AMMO;