#!/usr/bin/perl

#printf("enum\n{\n");

while(<>)
{
	$line = $_;
	($val,$name)=($line=~/\s*(\d*)\s*(.*)\b/);
#	$name=~s/\s/_/g;
#	$name=~s/,//g;
#	$name=~s/-//g;
#	print("OGE_USE_$name = $val,\n");
	printf("\"$name\",\n");
}

#printf("};\n");
