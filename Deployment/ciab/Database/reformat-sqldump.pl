# Crappy perl script to do some compaction of the gigantic sql export and to
# make some adjustments that turn it from a simple sql dump to a portable
# ciab dump: add user creations and grants, ensure that we don't export
# strat_cp, strat_facility, strat_object, strat_supercell or strat_link data.

local $/;
$_ = <>;
s!\r!!sg;
s!--\n!!sg;
s!INSERT INTO `strat_(supercell|cp|facility|object|link)`[^\n]+;\n!!g;
s!-- Dumping data for table `([^`]+)`\n(-- ORDER BY:[^\n]+\n)?\nLOCK TABLES `\1` WRITE;\n/[^/]+/;\n/[^/]+/;\nUNLOCK TABLES;\n\n!!sg;
s!\n\n\n+!\n!sg;
s!-- Table structure for table `[^`]+`\n\n!!sg;

s/^(-- Current Database: `[^`]+`\s*)$/${user_creation}\n$1/m;
print;
