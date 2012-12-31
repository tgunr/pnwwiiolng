######################################################################
# Member: Encapsulates a member variable.

class Member:
#{
	"""
		Encapsulates a member variable.
			strName		:-	Base name of the variable; "m_" will be added.
			strType		:-	The type of the variable.
			strBrief	:-	Brief description.
			strDefault	:-	Default value.
			strAccess	:-	public/private/protected.
			bPacked		:-	Whether value should be packed.
			bGet		:-	Set to True to generate a get() accessor.
			bSet		:-	Set to True to generate a set() accessor.
	"""

	#------------------------------------------------------------
	def __init__(self, strName, strType, strBrief=None, strDefault=None, strAccess='protected', bPacked=True, bGet=True, bSet=True):
	#{
		self.strName, self.strType, self.strDefault, self.strBrief, self.strAccess, self.bPacked, self.bGet, self.bSet = strName, strType, strDefault, strBrief, strAccess, bPacked, bGet, bSet

		self.bNeedsDefinition = False
		self.bFinalized = False
	#}

	#------------------------------------------------------------
	def _finalize(self):
		if self.bFinalized == False:
			###TODO: Set bNeedsDefinition = True for statics.
			self.bFinalized = True

	#------------------------------------------------------------
	def matches(self, strName, strType):
	#{
		if strName and strName != self.strName: return False
		if strType and strType != self.strType: return False
		return True
	#}

	#------------------------------------------------------------
	def declaration(self, strIndent, strSeparator):
	#{
		strOut = ""
		if self.strBrief: strOut += strSeparator + "/// @brief %s\n" % self.strBrief
		strOut += strSeparator + self.strAccess + ": " + self.strType + " m_" + self.strName + " ;\n"
		return strOut
	#}

	#------------------------------------------------------------
	def definition(self, strIndent, strSeparator, strPrefix=""):
	#{
		return strIndent + self.strType + " " + strPrefix + "m_" + self.strName + " = " + self.strDefault + " ;\n"
	#}

	#------------------------------------------------------------
	def default(self):
	#{
		return "m_%s(%s)" % (self.strName, self.strDefault if self.strDefault else "")
	#}
#}

