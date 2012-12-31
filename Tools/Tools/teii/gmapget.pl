#!/usr/bin/perl

use POSIX qw(ceil floor sinh atan);

$PI = 3.1415926535897;
$Q="q";
$R="r";
$S="s";
$T="t";

$level = $ARGV[4];
if($level == 0) { $level = 15;}

$lon1 = $ARGV[0];
$lat1 = $ARGV[1];
$lon2 = $ARGV[2];
$lat2 = $ARGV[3];

GetQuadsInRange($lon1,$lat1,$lon2,$lat2);

exit 0;

sub GetQuadsInRange
{
	my ($l,$b,$r,$t) = @_;
	
	$lb = GetQuadtreeAddress($l,$b, $level);
	$rb = GetQuadtreeAddress($r,$b, $level);
	$rt = GetQuadtreeAddress($r,$t, $level);
	$lt = GetQuadtreeAddress($l,$t, $level);

	for($i=0;$i<=$level;$i++)
	{
		if(substr($lb,$i,1) ne substr($rb,$i,1)) { last; }
		if(substr($lb,$i,1) ne substr($rt,$i,1)) { last; }
		if(substr($lb,$i,1) ne substr($lt,$i,1)) { last; }
		if(substr($rb,$i,1) ne substr($rt,$i,1)) { last; }
		if(substr($rb,$i,1) ne substr($lt,$i,1)) { last; }
		if(substr($rt,$i,1) ne substr($lt,$i,1)) { last; }
	}

	$topquad = substr($lb,0,$i);	

	GetSubQuads($topquad);
}

sub GetSubQuads
{
	my ($quad) = @_;

	if(length $quad > $level)
	{
		GetQuad($quad);
	}else
	{
		GetSubQuads("$quad$Q");
		GetSubQuads("$quad$R");
		GetSubQuads("$quad$S");
		GetSubQuads("$quad$T");
	}
}

sub GetQuadLatLon
{
	my($quad) = @_;

	$x = -1;
	$y = -1;
	$size = 2;
	for($i=0; $i < length $quad; $i++)
	{
		$s = substr($quad,$i,1);
		
		if($s eq "q")
		{
			$y += $size;
		}elsif($s eq "r")
		{
			$x += $size;
			$y += $size;
		}elsif($s eq "s")
		{
			$x += $size;
		}elsif($s eq "t")
		{
		}
		$size /= 2;
	}
	$l = $x * 180;
	$b = atan(sinh(($y+$size)*$PI)) * 180/$PI;
	
	$r = ($x+$size) * 180;
	$t = atan(sinh(($y+2*$size)*$PI)) * 180/$PI;

	return ($l,$b,$r,$t);
}

sub GetQuadtreeAddress
{
	my ($long, $lat, $level) = @_;
	$digits = $level;
	$x = (180.0 + $long) / 360.0;
	$y = -$lat * $PI / 180.0; 
	$y = 0.5 *log((1+sin($y))/(1-sin($y)));
	$y *= 1.0/(2 * $PI);
	$y += 0.5; 
	$quad = "t"; 
	@lookup = ('q', 'r', 't', 's');
	
	while($digits > 0) 
	{
		$x -= floor($x);
		$y -= floor($y);
		$i = ($x >= 0.5 ? 1 : 0) + ($y >= 0.5 ? 2 : 0);
		$quad = "$quad$lookup[$i]";		
		$x *= 2;
		$y *= 2;
		
		$digits--;
	}
	return $quad;
}

sub GetQuad
{
	my ($quad) = @_;
	$path = "kh?v=3&t=$quad";

	my($l,$b,$r,$t) = GetQuadLatLon($quad);

	if($r < $lon1) { return; }
	if($l > $lon2) { return; }
	if($t < $lat1) { return; }
	if($b > $lat2) { return; }
	
	if(-f "$quad.tif")
	{
		return;
	}else
	{
		system("wget \"http://kh.google.com/$path\"");
		system("mv \"$path\" $quad.jpg");
		system("convert -compress none $quad.jpg 0$quad.tif");
	}
	open(HEADER, ">header");
		
	$scalex = ($r-$l)/128.0;
	$scaley = ($t-$b)/128.0;
	$cx=($b+$t)/2.0;
	$cy=($l+$r)/2.0;
	
	print HEADER <<EOF;
Geotiff_Information:
   Version: 1
   Key_Revision: 1.0
   Tagged_Information:
      ModelTiepointTag (2,3):
         0                0                0
        $l $t       0
      ModelPixelScaleTag (1,3):
        $scalex $scaley   0
      End_Of_Tags.
 Keyed_Information:
	GTModelTypeGeoKey (Short,1): ModelTypeGeographic
	GTRasterTypeGeoKey (Short,1): RasterPixelIsArea
	GeographicTypeGeoKey (Short,1): GCS_WGS_84
      End_Of_Keys.
   End_Of_Geotiff.

GCS: 4326/WGS 84
Datum: 6326/World Geodetic System 1984
Ellipsoid: 7030/WGS 84 (6378137.00,6356752.31)
Prime Meridian: 8901/Greenwich (0.000000/  0d 0' 0.00"E)


Corner Coordinates:
Upper Left    (     $t, $l)
Lower Left    (     $b, $l)
Upper Right   (     $t, $r)
Lower Right   (     $b, $r)
Center        (    $cx , $cy )
EOF
	close(HEADER);

	system("geotifcp -g header 0$quad.tif $quad.tif");
	system("rm header 0$quad.tif $quad.jpg");
}
