#!/usr/bin/perl

foreach $layer (@ARGV)
{
	($layer)=($layer=~/([^.]*)/);

	print "$layer\n";

	system("ogr2ogr v0wor/$layer.shp v0eur/$layer.shp");
	system("ogr2ogr -update -append v0wor/$layer.shp v0sas/$layer.shp -nln $layer $layer");
	system("ogr2ogr -update -append v0wor/$layer.shp v0soa/$layer.shp -nln $layer $layer");
}