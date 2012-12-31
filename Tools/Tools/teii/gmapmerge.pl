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

	system("gdal_merge.py -ul_lr $l $t $r $b -o out.tif $quads\n");
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
	my($l,$b,$r,$t) = GetQuadLatLon($quad);

	if($r < $lon1) { return; }
	if($l > $lon2) { return; }
	if($t < $lat1) { return; }
	if($b > $lat2) { return; }

	if(-f "$quad.tif")
	{
		$quads = "$quads $quad.tif";
	}
}
