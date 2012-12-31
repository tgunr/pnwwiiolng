#!/usr/bin/env python
######################################################################
"""
	Implements the various classes which describe a <class> tag and
	it's components.
"""

from Function import Function
from Constructor import Constructor
from Operator import Operator
from Member import Member
from Interfaces import Interface

# #define guard string.
#self.strGuard = ("PN_OMG_" + (self.strName if not self.strNamespace else self.strNamespace + "_" + self.strName) + "_H").upper()

# Global dictionary of defined classes.
dclsClasses = dict()

######################################################################
# Class class: Encapsulates a C++ class definition.

class Class:
#{
	"""
		Encapsulates a C++ class definition.
	"""

	#------------------------------------------------------------
	def __init__(self, strNamespace, strName, strBrief=None, strDetail=None):
	#{
		""" Constructor """

		# Namespace we occupy.
		self.strNamespace = strNamespace
		# Name of this class.
		self.strName = strName

		self.strBrief = strBrief
		self.strDetail = strDetail

		self.clsParent = None

		# Full name will include the namespace if one was defined.
		self.strFullname = self.strName
		if self.strNamespace:
			self.strFullname = self.strNamespace + '::' + self.strName

		# List of all the types used by the class and it's children.
		self.astrTypesUsed = []

		# List of all the include files and what required them.
		self.astrIncludes = dict()

		# Base classes.
		self.astrBaseClasses = []

		# Documentation.
		self.astrDocumentation = []

		# Array of constructor functions.
		self.afnConstructors = \
		[
			Constructor(cClass=self, strArgs='', strBrief='Default Constructor')
		]
		self.strPackCtorArgs = "ACTION_OBJECT* restrict const a"

		# Destructor function.
		self.fnDestructor = None

		self.aclsClasses = [ ]

		# Operators.
		self.aopOperators = [ ]

		# Members variables.
		self.amMemberVars = [ ]

		# Member functions.
		self.afnMemberFns = [ ]

		# Members we are packing.
		self.astrPacking = [ ]

		# This gets set to True when we finally prepare the object for output.
		self.bFinalized = False
		self.bNeedsDefinition = False

		global dclsClasses
		dclsClasses[self.qualifiedName()] = self
	#}

	#------------------------------------------------------------
	def addConstructor(self, fnCtor):
	#{
		""" Add a constructor member function to the class. """

		self.afnConstructors.append(fnCtor)
	#}

	#------------------------------------------------------------
	def findConstructor(self, strArgs):
	#{
		""" Search the member function list for any constructors that match the arg list. """

		return [ fnCtor for fnCtor in self.afnConstructors if fnCtor.strArgs == strArgs ]
	#}

	#------------------------------------------------------------
	def addOperator(self, opOperator):
	#{
		""" Add the operator member functions. """

		self.aopOperators.append(opOperator)
	#}

	#------------------------------------------------------------
	def findOperator(self, strName=None, strArgs=None, strReturn=None):
	#{
		""" Search the member function list for any operators that match one or more specifications. """

		pass
	#}

	#------------------------------------------------------------
	def addFunction(self, fnFunc):
	#{
		""" Add a member function. """

		afnExisting = self.findFunction(strName=fnFunc.strName, strArgs=fnFunc.strArgs, strReturn=fnFunc.strReturn)
		if len(afnExisting) > 0:
			raise Exception("Duplicate member function %s %s::%s(%s)" % (fnFunc.strReturn, self.strName, fnFunc.strName, fnFunc.strArgs))
		self.afnMemberFns.append(fnFunc)

		self._unfinalize()
	#}

	#------------------------------------------------------------
	def findFunction(self, strName=None, strArgs=None, strReturn=None):
	#{
		""" Search the member function list for any member functions that match one or more specifications. """

		return [ fnFunc for fnFunc in self.afnMemberFns if fnFunc.matches(strName, strArgs, strReturn) ]
	#}

	#------------------------------------------------------------
	def addMember(self, mMember):
	#{
		""" Add a member variable. """

		amExisting = self.findMember(strName=mMember.strName, strType=mMember.strType)
		if len(amExisting) > 0:
			raise Exception("Duplicate member variable %s %s::%s" % (mMember.strType, self.strName, mMember.strName))

		self.amMemberVars.append(mMember)

		# Add Get accessor if required.
		if mMember.bGet:
			self.addFunction(Function(strName=mMember.strName, strArgs='', strCode='return m_%s ;' % mMember.strName, strReturn='const %s&' % mMember.strType, strAccess='public', bConst=True))
			self.addFunction(Function(strName=mMember.strName, strArgs='', strCode='return m_%s ;' % mMember.strName, strReturn='%s&' % mMember.strType, strAccess='public', bConst=False))

		# Add Set accessor if required.
		if mMember.bSet:
			self.addFunction(Function(strName=mMember.strName, strArgs='const %s& rhs' % mMember.strType, strCode='m_%s = rhs ;' % mMember.strName, strReturn='void', strAccess='public', bConst=False))

		# This object now needs a packer and a network constructor.
		if mMember.bPacked:
			print("%s adding packer %s" % ( self.qualifiedName(), mMember.strName) )
			self._addPacker("m_" + mMember.strName)

		self._unfinalize()
	#}

	#------------------------------------------------------------
	def _addPacker(self, strName):
	#{
		""" Add a packer for a named member. """

		# Ignore duplicates.
		if strName in self.astrPacking:
			return

		# Make sure a class-packer exists for our class itself.
		fn = self.findFunction(strName="packer")
		if not fn:
			fn = Function(strName = 'packer', strArgs = 'LOGICAL mode, '+self.strPackCtorArgs+', SINT32 size', strCode = '', strReturn = 'void', strBrief = 'User Implemented: Serialization routine.')
			self.addFunction(fn)
		elif len(fn) > 1:
			raise Exception("%s has multiple functions called 'packer'", self.qualifiedName)
		else:
			fn = fn[0]

		# Add the code to pack the specified member.
		fn.addCode("a->packer(mode, " + strName + ", 0) ; ")

		# Make sure we have a packing constructor.
		if not self.findConstructor(self.strPackCtorArgs):
			self.addConstructor(Constructor(cClass=self, strArgs=self.strPackCtorArgs, strCode='packer(TEUL_UNPACK, a) ;', strBrief='Construct from network'))

		self.astrPacking.append(strName)

		self._unfinalize()
	#}

	#------------------------------------------------------------
	def findMember(self, strName, strType=None):
	#{
		""" Search the member list for a matching variable name. """

		return [ mMember for mMember in self.amMemberVars if mMember.matches(strName, strType) ]
	#}

	#------------------------------------------------------------
	def addClass(self, clsClass):
	#{
		""" Add a sub-class. """

		aclsExisting = self.findClass(strName=clsClass.strName)
		if len(aclsExisting) > 0:
			raise Exception("Duplicate sub-class " % (clsClass.qualifiedName()))
		if clsClass.clsParent != None:
			raise Exception("Class %s already has a parent: %s" % (clsClass.qualifiedName(), clsClass.clsParent.qualifiedName()))

		self.aclsClasses.append(clsClass)
		clsClass.clsParent = self

		self._unfinalize()
	#}
	
	#------------------------------------------------------------
	def findClass(self, strName):
	#{
		""" Search the class list for a matching class. """
		return [ clsClass for clsClass in self.aclsClasses if self.strName == strName ]
	#}

	#------------------------------------------------------------
	def _unfinalize(self):
		""" Mark the object and it's parent as not finalized. """
		self.bFinalized = False
		if self.clsParent != None:
			self.clsParent._unfinalize()

	#------------------------------------------------------------
	def _finalize(self):
		""" Finalize the object for output. """

		if self.bFinalized:
			return

		# Finalize all of our members.
		for fnCtor in self.afnConstructors:
			fnCtor._finalize()
		for fnFunc in self.afnMemberFns:
			fnFunc._finalize()
		for mMember in self.amMemberVars:
			mMember._finalize()
		for clsClass in self.aclsClasses:
			clsClass._finalize()

		# Check through the components to look for anything
		# that needs a definition.
		# Note: Do this after finalizing all components,
		# so they have a chance to send up any changes.

		if self.bNeedsDefinition == False:
			for fnCtor in self.afnConstructors:
				if fnCtor.bNeedsDefinition: self.bNeedsDefinition = True
			for fnFunc in self.afnMemberFns:
				if fnFunc.bNeedsDefinition: self.bNeedsDefinition = True
			for mMember in self.amMemberVars:
				if mMember.bNeedsDefinition: self.bNeedsDefinition = True
			for clsClass in self.aclsClasses:
				if clsClass.bNeedsDefinition: self.bNeedsDefinition = True

		self.bFinalized = True

	#------------------------------------------------------------
	def declaration(self, strIndent="", strSeparator=""):
	#{
		""" Generate the C++ header file representation of this class. """

		self._finalize()

		strOut = ""

		if self.strNamespace:
			strOut += strIndent + "namespace %s\n" % self.strNamespace
			strOut += strIndent + "{\n"
			strOut += "\n"

		strOut += strIndent + "/// @%s %s\n" % (self.containerType(), self.qualifiedName())
		strOut += strIndent + "/// @brief %s\n\n" % (self.strBrief)

		###TODO: Insert detail documentation.

		strOut += strIndent + "%s %s\n" % (self.containerType(), self.strName)
		strOut += strIndent + "{\n"

		# Declare subclasses.
		if len(self.aclsClasses) > 0:
			strOut += strIndent + "//// Member classes.\n"
			for clsClass in self.aclsClasses:
				strOut += strIndent + "\tpublic:\n"
				strOut += clsClass.declaration(strIndent + "\t", strSeparator + "\t")

		for fnCtor in self.afnConstructors:
			strOut += fnCtor.declaration(strIndent + "\t", strIndent + "\t")

		strOut += "\n"

		for fnFunc in self.afnMemberFns:
			strOut += fnFunc.declaration(strIndent + "\t", strIndent + "\t")

		if len(self.amMemberVars) > 0:
			strOut += "\n"
			for mMember in self.amMemberVars:
				strOut += mMember.declaration(strIndent + "\t", strIndent + "\t")

		strOut += strIndent + "} ; // %s %s\n\n" % (self.containerType(), self.strName)

		if self.strNamespace:
			strOut += strIndent + "} // namespace %s\n\n" % self.strNamespace

		return strOut
	#}

	#------------------------------------------------------------
	def _brief(self, strIndent, strSeparator):
		""" Returns a brief summary of the class. """

		strBrief = self.strBrief if self.strBrief else ("%s %s" % (self.qualifiedName(), self.containerType()))
		return strSeparator + strBrief + "\n"

	#------------------------------------------------------------
	def _includes(self, strIndent, strSeparator):
		""" Returns a list of #includes for the class. """

		strIncludes = strSeparator + "// #includes required by class." + "\n"

		###TODO: Implement.

		return strIncludes

	#------------------------------------------------------------
	def _variables(self, strIndent, strSeparator):
		""" Returns definitions for static member-variables. """

		strVariables = strIndent + "// static variables." + "\n"

		###TODO: Implement.
		for mMember in self.amMemberVars:
			if mMember.bNeedsDefinition:
				strVariables += strIndent + "\t// " + mMember.strName + "\n"

		return strVariables + "\n"

	#------------------------------------------------------------
	def _code(self, strIndent, strSeparator):
		""" Returns the code for class member functions. """

		strCode = "" 
		strFnPrefix = self.strName + "::"

		# Start with constructors etc.
		for fnCtor in self.afnConstructors:
			if fnCtor.bNeedsDefinition:
				strFnCode = fnCtor.definition(strIndent, strIndent, strFnPrefix)
				if strFnCode:
					strCode += strFnCode + "\n"

		# Now member functions.
		for fnFunc in self.afnMemberFns:
			if fnFunc.bNeedsDefinition:
				strFnCode = fnFunc.definition(strIndent, strIndent, strFnPrefix)
				if strFnCode:
					strCode += strFnCode + "\n"

		if strCode:
			strCode = strIndent + ("// %s member functions.\n\n" % self.qualifiedName()) + strCode

		return strCode

	#------------------------------------------------------------
	def definition(self, strIndent="", strSeparator=""):
		""" Generate the C++ source file representation for a class. """

		self._finalize()

		if self.bNeedsDefinition == False:
			return ""

		strOut = ""
		strOut += self._brief(strIndent, strSeparator)
		strOut += self._includes(strIndent, strSeparator)
		strOut += self._variables(strIndent, strSeparator)
		strOut += self._code(strIndent, strSeparator)

		return strOut

	#------------------------------------------------------------
	#-- Descriptive functions.
	#------------------------------------------------------------

	#------------------------------------------------------------
	def containerType(self):
	#{
		""" Returns the name for this type of container, e.g. 'class' or 'struct' or 'union' """
		return "class"
	#}

	#------------------------------------------------------------
	def qualifiedName(self):
	#{
		""" Returns the name including namespace, if applicable. """

		strName = self.strName
		if self.clsParent:
			strName = self.clsParent.qualifiedName() + "::" + strName
		elif self.strNamespace:
			strName = self.strNamespace + "::" + strName
		return strName
	#}
#}

