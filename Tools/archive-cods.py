#! /bin/env python
######################################################################
# Short script to archive the CODs and executables.

import sys, os, shutil

# Get the build name
if len(sys.argv) != 2:
    raise Exception("Usage: %s <buildName>" % sys.argv[0])

buildName = sys.argv[1]

srcPath = "WW2\\proj\\win\\" + buildName
artPath = srcPath + "\\Artifacts"

if not os.path.exists(buildName):
    os.mkdir(buildName)

codArchive = "artifacts"
arcFormat = "zip"
arcFile = "%s.%s" % (codArchive, arcFormat)
destFile = buildName + "\\" + arcFile

if os.path.exists(arcFile):
    os.unlink(arcFile)

if os.path.exists(destFile):
    os.unlink(destFile)

shutil.make_archive(codArchive, arcFormat, artPath)

shutil.move(arcFile, buildName)

print("Generated %s\\%s" % (buildName, arcFile))
