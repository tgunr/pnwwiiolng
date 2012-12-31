#!/usr/bin/perl

$file = $ARGV[0];

system("listgeo $file > blah");

open (BLAH, "<blah");
open (BLAHOUT, ">blah2");
while(<BLAH>)
{
	print BLAHOUT $_;

	if($_=~/Keyed_Information/)
	{
print BLAHOUT <<EOF;
	GTModelTypeGeoKey (Short,1): ModelTypeGeographic
	GTRasterTypeGeoKey (Short,1): RasterPixelIsArea
	GeographicTypeGeoKey (Short,1): GCS_WGS_84
EOF
	}
	
	if($_=~/End_Of_Geotiff/)
{
print BLAHOUT <<EOF;
	GCS: 4326/WGS 84
	Datum: 6326/World Geodetic System 1984
	Ellipsoid: 7030/WGS 84 (6378137.00,6356752.31)
	Prime Meridian: 8901/Greenwich (0.000000/  0d 0' 0.00"E)
EOF

}
}
system("geotifcp -g blah2 $file 1$file");
system("rm blah blah2");