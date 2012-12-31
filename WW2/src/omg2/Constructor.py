######################################################################
# Constructor class: Encapsulates a constructor function.

from Function import Function

class Constructor(Function):
#{
	"""
		Encapsulates a Constructor, derived from Function.
			cClass		:-	Reference to the Class we are being built for.
			strArgs		:-	Arg list or None
			strBrief	:-	Brief description or None.
			strCode		:-	Body excluding enclosing {}s.
			strAccess	:-	public/private/protected.
	"""

	#------------------------------------------------------------
	def __init__(self, cClass, strArgs, strBrief=None, strCode=None, strAccess='public', bInline=True):
	#{
		Function.__init__(self, cClass.strName, strArgs, strBrief, strCode, None, strAccess, False, False, bInline, False)
		self.cClass = cClass
	#}

	#------------------------------------------------------------
	def matches(self, strName, strArgs, strReturn):
	#{
		return (strArgs == self.strArgs)
	#}

	#------------------------------------------------------------
	def _decoration(self, strIndent, strSeparator):
	#{
		""" Decorate with a constructor-list if applicable.  """

		if len(self.cClass.amMemberVars) <= 0:
			return ""
#		if self.strCode == None or self.strCode == '' or self.strCode == 'extern' or self.strCode == 'default' or self.strCode == 'deleted' or self.strCode == 'pure':
		if self.strCode == 'extern' or self.strCode == 'default' or self.strCode == 'deleted' or self.strCode == 'pure':
			return ""
		strOut = strSeparator + ": " + (", ").join([ mMember.default() for mMember in self.cClass.amMemberVars ])
		return strOut
	#}
#}

