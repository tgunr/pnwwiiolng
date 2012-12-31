#! /bin/env -e python
##################################################
# Process/manage versionNo.h
# -- Written for Python 2.7

import os, sys

versionPath = "WW2/proj"
relmode = False
upversion = False
manifestFile = "manifest"

def setUpversion(to_):
    global upversion
    if upversion:
        raise Exception("Multiple upversions specified on command line [%s]: not allowed." % to_)
    upversion = to_

def setRelmode(to_):
    global relmode
    if upversion:
        raise Exception("Multiple build modes specified on command line [%s]: not allowed." % to_)
    relmode = to_

argc = len (sys.argv)
for i in range(1, argc):
#{
    arg = sys.argv[i]
    if arg == "--build":
      upversion = "build"
    elif arg[0] == "-":
        raise Exception("Unrecognized command line switch: %s" % arg)
    else:
        versionPath = arg
#}

versionFile = versionPath + "/versionNo.h"

if os.path.exists(versionFile) == False:
    raise Exception("Unable to locate the versionNo.h header file: " + versionFile)

text = ""

with open(versionFile, 'r') as fh:
    # Read the existing information.
    for line in fh:
        text += line
        
def getValue(name):
    global text
    match = re.search(r"^#define\s+PN_WW2_"+name+"\s+(\d+)", text, flags=re.MULTILINE)
    if match:
        return int(match.group(1))
    else:
        raise Exception("Could not find "+name+" in versionNo.h")

def setValue(name, value):
    global text
    text = re.sub(r"^\s*(#define\s+PN_WW2_"+name+"\s+)\d+.*", r"\g<1>%u" % value, text, flags=re.MULTILINE)

def replaceDefine(name, value):
    global text
    stops = int((20 - len(name)) / 4) + 1
    tabs = "\t" * stops
    text = re.sub(r"^(#define\s+"+name+")\s+.*", r"\g<1>" + tabs + value, text, flags=re.MULTILINE)

import re
productNo = getValue("PRODUCT_NO")
versionNo = getValue("VERSION_NO")
patchNo   = getValue("PATCH_NO")
buildNo   = getValue("BUILD_NO")

if upversion == "build":
    if buildNo < 254:
        buildNo += 1
    else:
        patchNo += 1
        buildNo = 0

versionStr = "%u.%u.%u.%u" % (productNo, versionNo, patchNo, buildNo)
print("Version: %s" % versionStr)

oldText = text + ""

setValue("PRODUCT_NO", productNo)
setValue("VERSION_NO", versionNo)
setValue("PATCH_NO", patchNo)
setValue("BUILD_NO", buildNo)

replaceDefine("FILEVER", "%u,%u,%u,%u" % (productNo, versionNo, patchNo, buildNo))
replaceDefine("PRODUCTVER", "%u,%u,%u" % (productNo, versionNo, patchNo))
replaceDefine("STRFILEVER", "\"%u.%u.%u.%u\"" % (productNo, versionNo, patchNo, buildNo))
replaceDefine("STRPRODUCTVER", "\"%u.%u.%u\"" % (productNo, versionNo, patchNo))
replaceDefine("STRVER", "STRFILEVER")

if oldText != text:
    print("Updating versionNo.h")
    with open(versionFile, 'w') as fh:
        fh.write(text)
else:
    print("Re-using versionNo.h")

print("Generating manifest file")
with open(manifestFile, 'w') as fh:
    fh.write(versionStr)

