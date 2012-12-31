######################################################################
# Operator class: Encapsulates an operator function.

from Function import Function

class Operator(Function):
#{
	"""
		Encapsulates an operator, derived from Function.
			strName		:-	The operator name, excluding the word operator, e.g. "==".
			strArgs		:-	Arg list or None
			strBrief	:-	Brief description or None.
			strCode		:-	Body excluding enclosing {}s.
			strReturn	:-	Return type.
			strAccess	:-	public/private/protected.
			bConst		:-	(Member operators) constness of function.
			bVirtual	:-	(Member operators) virtual flag.
			bInline		:-	Should the body be inlined.
	"""

	#------------------------------------------------------------
	def __init__(self, strName, strArgs, strBrief=None, strCode=None, strReturn=None, strAccess='public', bConst=False, bVirtual=False, bInline=True):
	#{
		Function.__init__(self, 'operator %s' % strName, strArgs, strBrief, strCode, strReturn, strAccess, bConst, bVirtual, bInline, bStatic=False)
		self.strOperation = strName
	#}

	#------------------------------------------------------------
	def matches(self, strName, strArgs, strReturn):
	#{
		if strName and strName != self.strOperation: return False
		return Function.matches(self, strName, strArgs, strReturn)
	#}
#}

