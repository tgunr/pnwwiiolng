strCmt = "//!"

class DocParser:
#{
	""" Parses a <doc> container into doxygen comments. """

	def __init__(self, strFullname, elDoc, strPrefix=""):
	#{
		self.strFullname = strFullname
		self.elDoc = elDoc
		self.strPrefix = strPrefix
	#}

	def commentFormat(self, strLabel, strComment):
	#{
		"""
			Strips a section of text of it's leading indentation
			(so that comments can be nicely embedded in XML) and
			then adds the supplied prefix to all lines.

			Leading and trailing blank lines are stripped.
		"""

		if not strComment:
			return ""

		# Break into individual lines.
		astrLines = strComment.splitlines()

		# If there's only one line
		if len(astrLines) == 1:
			if strLabel:
				return "%s%s %s %s\n" % (self.strPrefix, strCmt, strLabel, strComment.strip())

		# Strip leading blanks.
		while len(astrLines) > 0 and len(astrLines[0].strip()) == 0:
			astrLines.pop(0)

		# Strip trailing lines.
		while len(astrLines) > 1 and len(astrLines[-1].strip()) == 0:
			astrLines.pop(-1)

		if len(astrLines) == 0:
			return ""

		# Find the number of tabs on the first line.
		ntabs = 0
		while astrLines[0][ntabs] == "\t":
			ntabs += 1

		if strLabel:
			strOut = "%s%s %s\n" % (self.strPrefix, strCmt, strLabel)

		for strLine in astrLines:
			if len(strLine) <= ntabs:
				strOut += "%s%s\n" % (self.strPrefix, strCmt)
			else:
				strOut += "%s%s %s\n" % (self.strPrefix, strCmt, strLine[ntabs:])

		return strOut
	#}

	def Parse(self):
	#{
		strBrief   = (" ".join(self.elDoc.findtext("brief").splitlines())).strip()	# Single line.
		strDetail  = self.elDoc.findtext("detail")
		strExample = self.elDoc.findtext("example")
		strReturns = self.elDoc.findtext("returns")
		strSeeAlso = self.elDoc.findtext("see-also")

		strOut = ""
		if self.strFullname:
			strOut += self.strPrefix + "//! %s\n" % self.strFullname
		strOut += self.strPrefix + "//! @brief %s\n" % strBrief

		strOut += self.commentFormat("@details", strDetail)
		strOut += self.commentFormat("@returns", strReturns)
		strOut += self.commentFormat("@example", strExample)

		if strSeeAlso:
			astrSeeAlso = strSeeAlso.strip().splitlines()
			astrItems = []
			for strLine in astrSeeAlso:
				for strItem in strLine.split(','):
					astrItems.append(strItem.strip())
			strOut += strPrefix + "//! @see %s\n" % ", ".join(astrItems)

		return strOut
	#}
#}
