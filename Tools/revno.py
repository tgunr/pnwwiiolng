#! /bin/env -e python
##################################################
# Script to generate a C file listing the current
# revision of a given source repository.
# -- Written for Python 2.7
# -- Requires PySVN extensions
#

revisionFile = "WW2/src/client_revision.h"
revisionDefine = "SVN_REVISION_NO"
revisionString = "SVN_REVISION_STR"
pathDefine = "SVN_PATH_STR"

#### Invoke a command and return the output
def output_from_command(cmd):
#{
	""" Return the standard output from a command as a string. """
	try:
		import subprocess
		result = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE).communicate()[0]
		return result
	except:
		raise Exception("Error occurred trying to run: "+cmd)
#}

#### Command line arguments: revno.py [<TopLevelPath>]
# Assume 0 or 1 argument;
#  0 arguments: Use the current working directory.
#  1 argument : Assumed to be the path to the top of the repository.

from sys import argv, exit
if len(argv) == 1:
	path = "./"
elif len(argv) == 2:
	path = argv[1]
	path = path.replace("\\", "/")
	if path.endswith("/") == False:
		path += "/"
else:
	exit("Usage: %s [repository path]" % (argv[0]))

#### Subversion client instanation.
# The Python SVN hooks are required.
# http://pysvn.tigris.org/
#  -> Downloads
#  -> Extension

usePySVN = 0
url = None
rev = None
suburl = None

#### Get working copy info.
# Look up SVN information about the path given

def pysvnQuery(path):
#{
	""" Get details of a repository using PySVN. """
	global url, rev, suburl

	import pysvn
	client = pysvn.Client()

	info = client.info(path)
	repos = info.repos + "/"
	url = info.url.replace(repos, "")
	rev = info.revision.number
	suburl = url.replace("branches", "").replace("tags", "").replace("/", "")
#}

#### Get working copy info.
# Look up SVN information using a shell query.

def shellQuery(path):
#{
	""" Get details of a repository using svn info. """
	global url, rev, suburl

	info = output_from_command("svn info %s" % path)

	import re
	repos = re.search(r"^Repository Root:\s*(\S+)", info, flags=re.MULTILINE).group(1).strip()
	url = re.search(r"^URL:\s*"+repos+r"/*(\S+)", info, flags=re.MULTILINE).group(1).strip()
	url = url.replace(repos, "")
	suburl = url.replace("branches", "").replace("tags", "").split("/")[0]

	rev = int(re.search(r"^Revision: (\d+)", info, flags=re.MULTILINE).group(1))
#}

#### Check existing header file for revision number

storedRev = 0
storedStr = ""

try:
	pysvnQuery(path)
	if url == None:
		shellQuery(path)
except:
	rev = 1000
	subUrl = "internal"

try:
	# Open the client_revision file
	with open(path + revisionFile, 'r') as f:
		# Read thru the lines.
		defines = [line for line in f if line.startswith("#define")]
		for line in defines:
			cols = line.split(" ")
			if cols[1] == revisionDefine:
				storedRev = int(cols[2].strip())
			elif cols[1] == revisionString:
				storedStr = cols[2].strip()
except (IOError):
	# We can ignore file not found errors.
	pass

#### Compare header file with actual revision number.
# If the file doesn't exist, or if it exists and
# has a different revision number than the working copy,
# we need to recreate the header file.

revStr = '"' + str(rev) + '"'
if rev != storedRev or storedStr != revStr:
	# Different revision numbers, update the revision file.
	with open(path + revisionFile, 'w') as f:
		f.write("#pragma once\n// CLIENT REVISION NUMBER, DO NOT CHECK IN\n")
		f.write("#define " + revisionDefine + " " + str(rev) + "\n")
		f.write("#define " + revisionString + " " + revStr + "\n")
		f.write("#define " + pathDefine + " \"" + str(suburl) + "\"\n")
	print("Rebuilding client_revision.h for %s revision %u" % (suburl, rev))
else:
	# Same same, just print a helpful note.
	print("client_revision.h upto date for %s revision %u" % (suburl, rev))

import sys
sys.exit(0)
