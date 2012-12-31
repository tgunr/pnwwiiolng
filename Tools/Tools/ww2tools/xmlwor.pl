#!/usr/bin/perl

print "<blitzwor>\n";
$ext="_ext";
while(<>)
{
	@args = split/,/;
	if($args[0] eq "LoadModel")
	{
		if($args[1]=~/terrain\\(.*)/)
		{
			$args[1]=~s/\\/\//g;
			($path)=($args[1]=~/terrain\/([^.]*).flt/);
			if($path eq "bld/flagtable1") { next; }
			print "<terrainmodel path=\"$path\"/>\n";
		}elsif($args[1]=~/vehicles\\(.*)/)
		{
			$args[1]=~s/\\/\//g;
			($path)=($args[1]=~/vehicles\/([^.]*).flt/);
			print "<vehiclemodel path=\"$path\"/>\n";
			print "<vehiclemodel path=\"$path$ext\"/>\n";
		}
	}elsif($args[0] eq "LoadCockpitModel")
	{
		if($args[1]=~/vehicles\\(.*)/)
		{
			$args[1]=~s/\\/\//g;
			($path)=($args[1]=~/vehicles\/([^.]*).flt/);
			print "<vehiclemodel path=\"$path\"/>\n";
		}		
	}
}
print "</blitzwor>\n";
