#!/usr/bin/perl

$name = $ARGV[0];
$area = $ARGV[1];
$mpp = $ARGV[2];

#($imgwidth, $imgheight)=($imgsize=~/(.*)x(.*)/);
($sx1,$sy1,$width,$height)=($area=~/(.*)x(.*)\+(.*)\+(.*)/);

$imgwidth = $width * 12800 / $mpp;
$imgheight = $height * 12800 / $mpp;
$x="x";
$imgsize="$imgwidth$x$imgheight";

system("./ww2genmap --area $area --size $imgsize --show renderedterrain --show buildings $name.png");

$sx2 = $sx1 + $width-1;
$sy2 = $sy1 + $height-1;

$lonsize = 180.0 / 8640000.0 * 12800.0;
$latsize = 70.1875 / 4211200.0 * 12800.0;

$left = $sx1 * $lonsize;
$right = ($sx2+1) * $lonsize;
$bottom = $sy1 * $latsize; 
$top = ($sy2+1) * $latsize; 	
$xdim = ($right-$left)/$imgwidth;
$ydim = ($top-$bottom)/$imgheight;
$vcenter = ($top+$bottom)/2.0;
$hcenter = ($left+$right)/2.0;

open OUT, ">$name.list";

print OUT <<META;
Geotiff_Information:
   Version: 1 
   Key_Revision: 1.0
   Tagged_Information:
      ModelTiepointTag (2,3):
         0                0                0                
         $left      $top      0                
      ModelPixelScaleTag (1,3):
         $xdim   $ydim   0                
      End_Of_Tags.
   Keyed_Information:
      End_Of_Keys.
   End_Of_Geotiff.
   
Corner Coordinates:
Upper Left    (       $left,     $top)
Lower Left    (       $left,     $bottom)
Upper Right   (       $right,    $top)
Lower Right   (       $right,    $bottom)
Center        (       $hcenter,    $vcenter)
META

close OUT;

`gdal_translate $name.png tmp$name.tif`;
`geotifcp -g $name.list tmp$name.tif $name.tif`;
`rm tmp$name.tif $name.png $name.list`;