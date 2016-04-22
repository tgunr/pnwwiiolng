#! /bin/env python
##################################################
# Script to generate a C file listing the current
# revision of a given source repository.
# -- Written for Python 2.7
#
# TODO: Was sloppily rewritten because of our
# switch to git. It works just fine, but it really
# needs to be cleaned up.

revisionFile = "WW2/src/client_revision.h"
revisionNo = "GIT_COMMIT_NO"
revisionString = "GIT_COMMIT_STR"

#### Invoke a command and return the output
def output_from_command(cmd):
#{
	""" Return the standard output from a command as a string. """
	try:
		import subprocess
		import sys

		result = str(subprocess.Popen(cmd.split(), stdout=subprocess.PIPE).communicate()[0], sys.stdout.encoding).strip('\n')
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

revNo = int(output_from_command("git log -n 1 --pretty=format:\"%h\"").replace('"', ''), 16)
revStr = output_from_command("git describe")

#### Check existing header file for revision number
storedNo = 0
storedStr = ""
try:
	# Open the client_revision file
	with open(path + revisionFile, 'r') as f:
		# Read thru the lines.
		defines = [line for line in f if line.startswith("#define")]
		for line in defines:
			cols = line.split(" ")
			if cols[1] == revisionNo:
				storedNo = int(cols[2].strip(), 16)
			elif cols[1] == revisionString:
				storedStr = cols[2].strip()[1:-1] # Strip quotes.
except (IOError):
	# We can ignore file not found errors.
	pass

#### Compare header file with actual revision number.
# If the file doesn't exist, or if it exists and
# has a different revision number than the working copy,
# we need to recreate the header file.

if storedNo != revNo or storedStr != revStr:
	# Different revision numbers, update the revision file.
	with open(path + revisionFile, 'w') as f:
		f.write("#pragma once\n// CLIENT REVISION NUMBER, DO NOT CHECK IN\n")
		f.write("#define " + revisionNo + " 0x%0.7x" % revNo + "\n")
		f.write("#define " + revisionString + " \"" + revStr + "\"\n")
	print("Rebuilding client_revision.h for commit %s" % (revStr))
else:
	# Same same, just print a helpful note.
	print("client_revision.h upto date for commit %s" % (revStr))

import sys
sys.exit(0)
