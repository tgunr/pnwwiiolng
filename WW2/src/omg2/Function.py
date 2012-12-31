######################################################################
# Function class: Base class for describing various types of function.

class Function:
#{
	"""
		Encapsulates some form of function.
			strName		:-		Common name of the function.
			strArgs		:-		List of arguments taken by function.
			strBrief	:-		Brief description of function.
			strCode		:-		Function body (not including enclosing {}s).
			strReturn	:-		Function return type (or None).
			strAccess	:-		public/private/protected.
			bConst		:-		True if this is a member function that should be declared const.
			bVirtual	:-		True if this is a member function that should be declared virtual.
			bInline		:-		Body should appear in declaration.
			bStatic		:-		True if this is a static member.

			bFinalized	:-		True when the finalize functions has been called.
			aStrCode	:-		Array of lines of code produced by finalize.
	"""

	#------------------------------------------------------------
	def __init__(self, strName, strArgs, strBrief=None, strCode=None, strReturn=None, strAccess='public', bConst=False, bVirtual=False, bInline=True, bStatic=False):
	#{
		self.strName, self.strArgs, self.strBrief, self.strCode, self.strReturn, self.strAccess, self.bConst, self.bVirtual, self.bInline, self.bStatic = strName, strArgs, strBrief, strCode, strReturn, strAccess, bConst, bVirtual, bInline, bStatic
		self.bFinalized = False
		self.bNeedsDefinition = False
		self.astrCode = []
	#}

	#------------------------------------------------------------
	def matches(self, strName, strArgs, strReturn):
	#{
		""" Returns True/False whether this function matches the arg list.  """

		if strName and strName != self.strName: return False
		if strArgs and strArgs != self.strArgs: return False
		if strReturn and strReturn != self.strReturn: return False
		return True
	#}

	#------------------------------------------------------------
	def _brief(self, strIndent, strSeparator):
	#{
		""" Returns the documentation for this function. """

		if not self.strBrief:
			return ""

		return strSeparator + "//! @brief %s" % (self.strBrief)
	#}

	#------------------------------------------------------------
	def _accessLevel(self, strIndent, strSeparator):
	#{
		""" Returns the access decorator for this function. """

		return strSeparator + self.strAccess + ":"
	#}

	#------------------------------------------------------------
	def _decoration(self, strIndent, strSeparator):
	#{
		""" Returns decoration for this function.  """

		strOut = ""
		strOut += strSeparator + "static" if self.bStatic else ""
		strOut += strSeparator + "const" if self.bConst else ""
		return strOut
	#}

	#------------------------------------------------------------
	def _returnType(self, strIndent, strSeparator):
	#{
		""" Returns the return type for this function. """

		return strSeparator + self.strReturn if self.strReturn else ""
	#}

	#------------------------------------------------------------
	def _function(self, strIndent, strSeparator, strPrefix=""):
	#{
		""" Returns function name and arguments. """

		return strSeparator + strPrefix + self.strName + "(" + self.strArgs + ")"
	#}

	#------------------------------------------------------------
	def _code(self, strIndent, strSeparator):
	#{
		""" Returns the code for this function. """
		# Handle empty code case.
		if not self.astrCode:
			return ""

		# Handle single line code case by appending to
		# existing line.
		if len(self.astrCode) == 1:
			return strSeparator + self.astrCode[0]

		# Handle multiple-line case by spreading out
		# onto multiple CR separated lines.
		lines = [ strIndent + line for line in self.astrCode ] ;
		return "\n" + "\n".join(lines)
	#}

	#------------------------------------------------------------
	def addCode(self, strCode):
		if self.strCode == "extern" or self.strCode == "pure":
			raise Exception("Tried to add code to function %s which is extern." % (self.strName, self.strCode))
		if self.strCode.strip() == ";":
			self.strCode = strCode
		else:
			if self.strCode:
				self.strCode += "\n"
			self.strCode += strCode

		self.bFinalized = False

	#------------------------------------------------------------
	def _finalize(self):
		""" Finalizes the object. """

		if self.bFinalized == True:
			return

		###TODO: Rather than forcing these values,
		###TODO: Should probably generate a meaningful warning?
		###TODO: Just don't throw errors, since we can correct.
		if self.strCode == None or self.strCode == "":
			self.astrCode = [ '{}' ]
			self.bInline = True
		elif self.strCode == 'extern':
			self.astrCode = [ ';' ]
			self.bInline = True
		elif self.strCode == 'default' or self.strCode == 'deleted':
			self.astrCode = [ "= %s ;" % self.strCode ]
			self.bInline = True
		elif self.strCode == 'pure':
			self.astrCode = [ "= 0 ;" ]
			self.bInline = True
		elif self.strCode.find("\n") < 0:
			if self.bInline == True:
				self.astrCode = [ "{ " + self.strCode + " }" ]
			else:
				self.astrCode = [ "{", "\t" + self.strCode + "",  "}" ]
		else:
			self.astrCode = [ "{" ]
			for strLine in self.strCode.splitlines():
				self.astrCode.append("\t" + strLine.strip())
			self.astrCode.append("}")

		if self.bInline == False and self.strCode != 'extern':
			self.bNeedsDefinition = True

		self.bFinalized = True

	#------------------------------------------------------------
	def declaration(self, strIndent, strSeparator):
	#{
		""" Returns the declarative (i.e. header-suitable) version of this object. """

		self._finalize()

		strOut = ""

		strOut = self._brief(strIndent, strSeparator)
		###TODO: Add full doc.
		if strOut:
			strOut += "\n" + strIndent
		else:
			strOut += strSeparator

		strOut += self._accessLevel(strIndent, "")
		strOut += self._returnType(strIndent, " ")
		strOut += self._function(strIndent, " ") + self._decoration(strIndent, " ")

		if self.bInline == True:
			strOut += self._code(strIndent, " ")

		strOut += "\n"

		return strOut
	#}

	#------------------------------------------------------------
	def definition(self, strIndent, strSeparator, strPrefix=""):
	#{
		""" Returns the dfefinitive (i.e. CPP-file suited) version of this object. """

		self._finalize()

		# Extern routines have to be defined by the user someplace else.
		if self.bInline or self.strCode == 'extern':
			return ""

		strOut = self._returnType(strIndent, strIndent) + "\n"
		strOut += self._function(strIndent, strIndent, strPrefix) + self._decoration(strIndent, " ")

		strOut += self._code(strIndent, " ")

		strOut += "\n"

		return strOut
	#}
#}

