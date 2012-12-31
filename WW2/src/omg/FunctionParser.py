from DocParser import DocParser

class FunctionParser:
#{
	""" Parses a <function> container. """

	def __init__(self, elFunc, strIndent):
	#{
		self.elFunc = elFunc
		self.strName = elFunc.get("name").strip()
		self.strReturns = elFunc.get("return-type").strip()
		self.strArguments = elFunc.get("arguments")
		if self.strArguments:
			self.strArguments = self.strArguments.strip()
		if not self.strArguments:
			self.strArguments = ""
		self.strConst = " const" if elFunc.get("const") == "true" else ""
		self.strIndent = strIndent
	#}

	def codeFormat(self, strCode):
	#{
		"""
			Strips a section of text of it's leading indentation
			(so that code can be nicely embedded in XML) and
			then adds the supplied prefix to all lines.

			Leading and trailing blank lines are stripped.
		"""

		if strCode == None:
			return " ;"

		# Break into individual lines.
		astrLines = strCode.splitlines()

		# Strip leading blanks.
		while len(astrLines) > 0 and len(astrLines[0].strip()) == 0:
			astrLines.pop(0)

		# Strip trailing lines.
		while len(astrLines) > 1 and len(astrLines[-1].strip()) == 0:
			astrLines.pop(-1)

		if len(astrLines) == 0:
			return " ;"

		# Find the number of tabs on the first line.
		ntabs = 0
		while astrLines[0][ntabs] == "\t":
			ntabs += 1

		strOut = "\n"
		strOut += self.strIndent + "{\n"

		for strLine in astrLines:
			strOut += self.strIndent + "\t" + strLine[ntabs:] + "\n"

		strOut += self.strIndent + "}"

		return strOut
	#}

	def Parse(self):
	#{
		dp = DocParser(None, self.elFunc.find("doc"), self.strIndent)

		strOut = dp.Parse()

		strCode = self.codeFormat(self.elFunc.findtext("code"))

		strOut += "%s%s %s(%s)%s%s\n" % (self.strIndent, self.strReturns, self.strName, self.strArguments, self.strConst, strCode)

		return strOut
	#}
#}
