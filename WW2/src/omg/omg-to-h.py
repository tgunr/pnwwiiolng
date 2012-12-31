#! /usr/bin/env python

"""
	Object Model Generator	v1.0

	Auth: Oliver 'kfs1' Smith <osmith@playnet.com>
	Created: Tue Nov 30 10:41:24 CST 2010

	Translates OMG XML specifications into C++ header files
	containing a class definition.

	XML is validated against "omg.dtd".
"""

# Imports/Dependencies.
try:
	from lxml import etree
except ImportError:
	import xml.etree.cElementTree as etree
from optparse import OptionParser
import sys
import os
import io

from Defaults import *
from ClassParser import ClassParser
from VarGroupParser import VarGroupParser
from DocParser import DocParser

# Argument parsing.
parser = OptionParser(description="Build a header file from an OMG definition.", usage="omg-to-h.py [--dtd=dtd FILEname] <source omg file>.omg <destination file>.h")
parser.add_option('--dtd', help='Specify the DTD file to use', default='omg.dtd')
#parser.add_option('infile', help='Specify the OMG file to read')
#parser.add_option('outfile', help='Specify the .h file to write to')
(opts, args) = parser.parse_args()
if len(args) != 2:
	parser.error("incorrect number of arguments")
opts.infile, opts.outfile = args[0], args[1]

## Base class for the OMGParser.

class OMGParser:
#{
	"""Reads OMG XML specifications, outputs C++ .h files"""

	###############################################################################
	## Stock members (do not overload)
	###############################################################################

	##----------------------------------------------------------------------
	def __init__(self, strSource):
	#{
		# Load the document body.
		elRoot = etree.parse(strSource).getroot()

		elDTD = None
		try:
			# Load the DTD.
			elDTD = etree.DTD(opts.dtd)
		except:
			pass

		if elDTD:
			# Validate the XML against the DTD.
			if not elDTD.validate(elRoot):
			#{
				for strErr in elDTD.error_log.filter_from_errors()[0:5]:
					print(strErr)
				raise Exception("DTD Validation failed")
			#}

		# Extract base values from the XML.
		self.elClass = elRoot

		# Names
		self.strNamespace = self.elClass.get('namespace')
		self.strName = self.elClass.get('name')
		self.strFullname = self.strName
		if self.strNamespace: self.strFullname = self.strNamespace + '::' + self.strName
		self.strGuard = ("PN_OMG_" + (self.strName if not self.strNamespace else self.strNamespace + "_" + self.strName) + "_H").upper()

		# Source file name.
		self.strSrc = strSource
	#}

	##----------------------------------------------------------------------
	def baseIncludeFiles(self):
	#{
		""" Overridable list of stock includes for this class-type. """
		return [ 'wwii/wwiiSyncVar.h' ], { 'wwii/wwiiSyncVar.h' : { 'VarGroup' : 1, 'SyncVar' : 1 } }
	#}

	##----------------------------------------------------------------------
	def _getTypesUsed(self):
	#{
		""" Returns a list of types used by the sub-members. """

		# Generate a unique list of the types.
		astrTypes = dict((elMember.get("type"), 1) for elMember in self.elClass.findall("*/member"))

		# Return just the keys.
		return astrTypes.keys()
	#}

	##----------------------------------------------------------------------
	def _includes(self):
	#{
		"""
			Scan the var-group members for types that invoke dependencies,
			and generate a list of the headers this requires along with
			which types invoked those includes, so we can list which
			headers and why we include them in the output file.
		"""

		strOut = ""

		astrIncludeFiles, astrIncludes = self.baseIncludeFiles()

		for strType in self._getTypesUsed():
		#{
			# Ignore types that don't have a dependency.
			if not strType in astrTypeToHeader:
				continue

			strHeader = astrTypeToHeader[strType]

			if not (strHeader in astrIncludes):
				# New include.
				astrIncludeFiles.append(strHeader)
				astrIncludes[strHeader] = { strType:1 }
			else:
				# Increment the count of types invoking the dependency.
				astrIncludes[strHeader][strType] += 1
		#}

		for strHeader in astrIncludeFiles:
			strOut += "// For %s\n#include \"%s\"\n" % (", ".join(astrIncludes[strHeader].keys()), strHeader)

		return strOut + "\n"
	#}

	##----------------------------------------------------------------------
	def _generateCPP(self):
	#{
		""" Generate C++ header file code based on everything we've loaded etc. """

		## include guard.
		# strOut  = "#ifndef %s\n" % self.strGuard
		# strOut += "#define %s 1\n\n" % self.strGuard
		strOut = "#pragma once\n"

		## indicate this is automatically generated.
		strOut += "////////////////////////////////////////////////////////////////////////////////////////\n"
		strOut += "// @file %s\n" % opts.outfile
		strOut += "// @attention AUTOMATICALLY GENERATED FROM omg/%s BY omg/omg-to-h.py\n" % self.strSrc
		strOut += "// @attention DO NOT MODIFY THIS FILE DIRECTLY edit omg/%s instead.\n" % self.strSrc
		strOut += "////////////////////////////////////////////////////////////////////////////////////////\n\n"

		## include files.
		strOut += self._includes()

		strOut += "#if defined(_MSC_VER)\n# pragma warning(disable : 4355) // C4355: 'this': used in base member initializer list.\n#endif\n\n"
		parser = ClassParser(self.strSrc, self.elClass)
		strOut += parser.Parse()

		#strOut += "#endif // %s\n" % self.strGuard

		return strOut
	#}

	###############################################################################
	## Overridable functions.

	##----------------------------------------------------------------------
	def write(self, strOutfile):
	#{
		""" Write the C++ code to the specified .h file. """

		# Generate C++ code.
		strOut = self._generateCPP()
		try:
			strOut = unicode(strOut)
		except NameError:
			pass

		# Write to file.
		with io.open(strOutfile, 'w', buffering=1, encoding='UTF-8') as fhOutfile:
			fhOutfile.write(strOut)
	#}
#} OMGParser

omg = OMGParser(opts.infile)
omg.write(opts.outfile)

