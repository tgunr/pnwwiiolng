#!/usr/bin/perl

%wantedturretvars=( 
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
	%multicrewpos=();
	@turretvars = ({},{},{},{},{},{},{},{} );
	print "$filename\n";
	
	($name)=($filename=~/(.*)\.pos/);
	open TURRET, "<$name.turret";
	$turretnum = 1;
	while(<TURRET>)
	{
		chomp;
	
		($pos, $multipos)=($_=~/multicrewpos\s*(\S*)\s*(\S*)/);
		if($pos ne "" && $multipos ne "")
		{
			$multicrewpos{$pos} = $multipos;
			next;
		}

		($key, $val)=($_=~/(\S*)\s*=\s*(\S*)/);
		if($key eq "" || $val eq "") { next; }
		if(substr($key,0,1) eq "#") { next; }

		$val=~s/\\/\//g;
		
		if($key eq "position")
		{
			$position = $val;
			$turretvars[$position]{"turretnum"} = $turretnum;
			$turretnum++;
		}elsif($wantedturretvars{$key} != 1)
		{
			next;
		}
		
		$turretvars[$position]{$key} = $val;
	}
	close TURRET;

	$position = 0;
	$postag="";
	$posbody="";

	open IN, "<$filename";

	open OUT, ">$filename.xml";
	print OUT "<?xml version='1.0'?>\n<!DOCTYPE positions SYSTEM \"position.dtd\">\n";
	print OUT "<positions>\n";
	while(<IN>)
	{
		$_ =~ s/\s+$//;
		
		($groupnum,$msg) = ($_=~/groupmsg\s*([0-9]*)\s*(.*)/);
		if($groupnum ne "")
		{
			$posbody= "$posbody\t<groupmsg id=\"$groupnum\" value=\"$msg\"/>\n";
		}
		
		($key, $val)=($_=~/(\S*)\s*=\s*(\S*)/);
		if($key eq "" || $val eq "") { next; }
		
		if(substr($key,0,1) eq "#") { next; }
		
		$val=~s/\\/\//g;

		($name)=($val=~/sounds\/(.*)\.wav/);
		if($name ne "") {$val = $name;}
		
		if($key eq "position")
		{
			if($postag ne "")
			{
				print OUT $postag;
				if($posbody ne "")
				{
					print OUT ">\n$posbody</position>\n";
				}else
				{
					print OUT "/>\n";
				}
			}
			$position = $val;
			$postag = "<position id=\"$position\"\n";
			$posbody = "";
			if($multicrewpos{$position} ne "" )
			{
				$postag = "$postag\tmulticrewpos=\"$multicrewpos{$position}\"\n";
			}
			$href = $turretvars[$position];
			foreach $key (keys %$href)
			{
				$postag = "$postag\t$key=\"$$href{$key}\"\n";
			}
			
		}else
		{
			$postag = "$postag\t$key=\"$val\"\n";
		}
	}

	if($postag ne "")
	{
		print OUT $postag;
		if($posbody ne "")
		{
			print OUT ">\n$posbody</position>\n";
		}else
		{
			print OUT "/>\n";
		}
	}
	print OUT "</positions>\n";

	close IN;
	close OUT;
}