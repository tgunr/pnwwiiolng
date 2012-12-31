#!/usr/bin/perl

foreach $file (@ARGV)
{	
	($ns,$lat,$ew,$lon)=($file=~/([A-Z])([0-9]*)([A-Z])([0-9]*)/);
	
	$ULXMAP = $lon + 1.0 - 1.0;
	$ULYMAP= $lat + 1.0;

	if($ns eq "S")
	{
		$ULYMAP = -$ULYMAP;
	}
	
	if($ew eq "W")
	{
		$ULXMAP = -$ULXMAP;
	}
	
	$name = "$ns$lat$ew$lon";
 	$hdr = "BYTEORDER M
LAYOUT BIL
NROWS 1201
NCOLS 1201
NBANDS 1
NBITS 16
BANDROWBYTES 2402
TOTALROWBYTES 2402
BANDGAPBYTES 0
NODATA -32768
ULXMAP $ULXMAP
ULYMAP $ULYMAP
XDIM 0.000833333333333
YDIM 0.000833333333333";

	`echo "$hdr" > $name.hdr`;
# 	`unzip $file`;
#	`mv $name.hgt $name.bil`;

}