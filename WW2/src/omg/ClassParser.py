###############################################################################
## VarGroup element parser.

from Defaults import *
from DocParser import DocParser
from FunctionParser import FunctionParser
from VarGroupParser import VarGroupParser

class ClassParser:
#{

	def __init__(self, strSource, elRoot):
	#{
		global strDefaultCode, strDefaultPacker

		self.strSrc = strSource
		self.elRoot = elRoot

		# Namespace we occupy.
		self.strNamespace = self.elRoot.get('namespace')
		# Name of this class.
		self.strName = self.elRoot.get('name')

		# Full name will include the namespace if one was defined.
		self.strFullname = self.strName
		if self.strNamespace:
			self.strFullname = self.strNamespace + '::' + self.strName

		self.aelMembers = self.elRoot.findall('member')

		# List of var-groups
		self.aelVarGroups = elRoot.findall("var-group")

		self.clpParent = self
		self.astrTypesUsed = []
		self.astrBaseClasses = []
		self.strDocumentation = ''
		self.astrConstructors = { '()' : '=default' }
		self.astrCopyOperators = { '()' : '=default' }
		self.strDestructorCode = ''
		self.strDefaultCode = strDefaultCode + "\n"
		self.astrMemberVariables = dict()
		self.astrMemberFunctions = dict()
		self.strPacker = strDefaultPacker
	#}

	##----------------------------------------------------------------------
	def _documentation(self):
	#{
		parser = DocParser("@class %s" % self.strFullname, self.elRoot.find("doc"))
		return parser.Parse() + "\n//! @attention This class is automatically generated from %s\n\n" % self.strSrc
	#}

	##----------------------------------------------------------------------
	def _includes(self):
		astrIncludeFiles, astrIncludes = [], dict()

		for strType in self.astrTypesUsed:
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

	##----------------------------------------------------------------------
	def baseclass(self):
	#{
		""" Overridable list of base classes for this class-type. """
		return ""
	#}

	##----------------------------------------------------------------------
	def defaultConstructor(self):
	#{
		""" Returns definitions for constructors. """
		## Build a list of the var groups that need constructing.

		# Start the ctor list with the base class constructor.
		astrMemberCtors = [ 'm_lastModified(0ULL)' ]
		for elVarGroup in self.aelVarGroups:
			astrMemberCtors.append("m_%s(m_lastModified)" % elVarGroup.get("name").lower())

		strMemberCtors = "\n\t\t, ".join(astrMemberCtors)

		strOut = """\
	// Default constructor.
	public:
	%s()
		: %s
		{
		}

"""
		return strOut % (self.strName, strMemberCtors)
	#}

	##----------------------------------------------------------------------
	def stockCode(self):
	#{
		return self.strDefaultCode + strDefaultPackerComment + self.strPacker + "\n"
	#}

	##----------------------------------------------------------------------
	def destructor(self):
	#{
		""" Returns definitions for the destructor. """
		return ""
	#}

	##----------------------------------------------------------------------
	def copyOperator(self):
	#{
		""" Returns definitions for the copy operator. """
		return ""
	#}

	##----------------------------------------------------------------------
	def memberVariables(self):
	#{
		""" Returns definitions for the member variables. """

		strOut = "\t// Members\n\n"

		## Build a list of the var-groups.
		for elVarGroup in self.aelVarGroups:
			parser = VarGroupParser(elVarGroup, self)
			strOut += parser.Parse()

		## Build a list of normal members.
		for elMember in self.aelMembers:
			strType = elMember.get("type")
			strOut += "\t//! %s\n" % elMember.get("brief")
			strOut += "\t%s %s ;\n" % (strType, elMember.get("name"))

		return strOut
	#}

	##----------------------------------------------------------------------
	def memberFunctions(self):
	#{
		""" Returns definitions for functions at the class level. """

		strOut = ""

		for elFunc in self.elRoot.findall('function'):
			fp = FunctionParser(elFunc, "\t")
			strOut += "\n"
			strOut += fp.Parse()

		if len(strOut) > 0:
			strOut = "\t// Member Functions.\n\n" + strOut

		return strOut
	#}

	##----------------------------------------------------------------------
	def addTypeUsed(self, typeName):
		if not typeName in self.astrTypesUsed:
			self.astrTypesUsed.append(typeName)

	##----------------------------------------------------------------------
	def ProcessContents(self):
	#{
		global strDefaultVarGroupCode

		if self.aelVarGroups:
			self.addTypeUsed('SyncVar')
			self.addTypeUsed('VarGroup')

		iNumVarGroups = len(self.aelVarGroups)

		if iNumVarGroups > 0:
			iBitsRequired = (iNumVarGroups + 7) & ~7
			self.strDefaultCode += (strDefaultVarGroupCode % iBitsRequired) + "\n"
			# Change the packer of the clpParent.
			self.clpParent.strPacker = """\
	public: bool packer(LOGICAL mode, ACTION_OBJECT* a, const TeulTimeMS& lastPack) ;
"""
	#}

	##----------------------------------------------------------------------
	def Parse(self):
	#{
		strOut = ""

		## open namespace.
		if self.strNamespace:
			strOut += "namespace %s\n{\n\n" % self.strNamespace

		## Include documentation for the class.
		strOut += self._documentation()

		self.ProcessContents()

#		strOut += self._includes()

		## Class declaration and optional base classes.
		strBaseclass = self.baseclass()
		strOut += "class " + self.strName
		if strBaseclass:
			strOut += " : " + strBaseclass
		strOut += "\n{\n"

		## Constructors.
		strOut += self.defaultConstructor()

		## Destructor.
		strOut += self.destructor()

		## Copy operator.
		strOut += self.copyOperator()

		## Inject any stock code.
		strOut += self.stockCode()

		## Member variables.
		strOut += self.memberVariables()

		## Member functions.
		strOut += self.memberFunctions()

		## Close class definition.
		strOut += "} ; // class %s\n\n" % self.strName

		## Close namespace.
		if self.strNamespace:
			strOut += "} // namespace %s\n\n" % self.strNamespace

		return strOut
	#}

#}

