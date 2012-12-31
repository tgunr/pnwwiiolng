#!/usr/bin/perl

print `tar xf xmlcvt.tar`;

$files = `find data/ -name "*.control" | xargs ./xmlcontrol.pl`; print $files; $all = "$all$files";
$files = `find data/ -name "*.hit" | xargs ./xmlhit.pl`; print $files; $all = "$all$files";
$files = `find data/ -name "*.loadout" | xargs ./xmlloadout.pl`; print $files; $all = "$all$files";
$files = `find data/ -name "*.pos" | xargs ./xmlpos.pl`; print $files; $all = "$all$files";
$files = `find data/ -name "*.state" | xargs ./xmlstate.pl`; print $files; $all = "$all$files";
$files = `find data/ -name "*.turret" | xargs ./xmlturret.pl`; print $files; $all = "$all$files";
$files = `./xmlammo.pl data/ammo/ammo.txt`; print $files; $all = "$all$files";
$files = `./xmlweapon.pl data/weapons/weapons.txt`; print $files; $all = "$all$files";

print `./xmlwor.pl < data/blitzWor.csv > data/blitzWor.xml`; 
$files = "data/blitzWor.csv\n";
print $files; $all = "$all$files";

$all=~s/\n/ /g;

print `tar --create --ignore-failed-read --file xmlcvt.tar $all`;
print `rm $all`;