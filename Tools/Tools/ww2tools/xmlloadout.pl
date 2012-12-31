#!/usr/bin/perl

foreach $filename (@ARGV)
{
print "$filename\n";

open IN, "<$filename";

($name)=($filename=~/(.*)\.loadout/);
open JMM, "<$name.jmm";

@rounds = ();
@expend = ();

while(<JMM>)
{
	($numrounds, $expendable) = split;
	push @rounds, $numrounds;
	push @expend, $expendable ? "yes" : "no";
}

close JMM;

open OUT, ">$filename.xml";
print OUT "<?xml version='1.0'?>\n<!DOCTYPE loadouts SYSTEM \"vehicle.dtd\">\n";
print OUT "<loadouts>\n";

$loadoutnum = $mountnum = 0;

while(<IN>)
{
	chomp;
	($key, $val)=($_=~/(\S*)\s*=\s*(\S*)/);
	
	if($key eq "" || $val eq "") { next; }
	if(substr($key,0,1) eq "#") { next; }
	
	if(($id)=$key=~/^sound(.*)/)
	{
		$val=~s/\\/\//g;
		($name)=($val=~/sounds\/(.*)\.wav/);
		if($name ne "") { $val = $name;}
		print OUT "<sound id=\"$id\"\ path=\"$val\"/>\n";
	}elsif($key eq "loadout")
	{
		if($loadoutnum)
		{
			print OUT "</loadout>\n";
		}
		
		$loadoutnum = $val;

		print OUT "<loadout id=\"$loadoutnum\">\n";
		
	}elsif($key eq "mountnum")
	{	
		if($mountnum) { printmount(); }
		%mountattr=();
		@mountval=();
		@weapon=();
		@weaponobject=();
		$mountnum = $val;
	}elsif($key eq "weapon")
	{
		push @weapon, $val;
	}elsif($key eq "weaponobject")
	{
		push @weaponobject, $val;
	}else
	{
		($mountkey)=($key=~/^mount(.*)/);
		if($mountkey ne "") { $key=$mountkey; }
		
		if($key eq "sound") {($val)=($val=~/^sound(.*)/);}
		$mountattr{$key} = $val;
	}
}	

if($mountnum) { printmount(); }

if($loadoutnum)
{
	print OUT "</loadout>\n";
}

print OUT "</loadouts>\n";

close IN;
close OUT;
}

exit;

sub printmount
{
	print OUT "<mount id=\"$mountnum\" ";
	if($#rounds > 0)
	{
		print OUT "numrounds=\"$rounds[$mountnum-1]\" expendable=\"$expend[$mountnum-1]\" ";
	}
	
	foreach $key (keys %mountattr) 
	{	
		print OUT "$key=\"$mountattr{$key}\" ";
	}
	print OUT ">\n";

	for($i=0; $i <= $#weapon; $i++)
	{
		print OUT "\t<weapon id=\"$weapon[$i]\" object=\"$weaponobject[$i]\"/>\n";
	}
	
	print OUT "</mount>\n";
}