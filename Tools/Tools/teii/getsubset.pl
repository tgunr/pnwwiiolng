#!/usr/bin/perl

$lon1 = $ARGV[0];
$lat1 = $ARGV[1];
$lon2 = $ARGV[2];
$lat2 = $ARGV[3];
$name = $ARGV[4];


for($lat = $lat1; $lat <= $lat2; $lat++)
{
	for($lon = $lon1; $lon <= $lon2; $lon++)
	{
		$filename = sprintf("N%dE%03d", $lat, $lon);
	print "$filename\n";			
		if(!(-e "$filename.bil"))
		{
			if(-e "$filename.hgt.zip")
			{
				`unzip $filename.hgt.zip`;

				$err = system("mv $filename.hgt $filename.bil");
				if($err != 0)
				{
					$err = system("mv $filename.hgt.bin $filename.bil");
				}
			}
		}
		
		if(-e "$filename.bil")
		{
			$files = "$files $filename.bil";
		}
	}
}

`gdal_merge.py $files -o $name`;

 	$hdr = "BYTEORDER M
LAYOUT BIL
NROWS 3601
NCOLS 3601
NBANDS 1
NBITS 16
BANDROWBYTES 7202
TOTALROWBYTES 7202
BANDGAPBYTES 0
NODATA -32768
ULXMAP $ULXMAP
ULYMAP $ULYMAP
XDIM 0.0002777777777777778
YDIM 0.0002777777777777778";

	`echo "$hdr" > $name.hdr`;
