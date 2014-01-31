###############################################################################
## VarGroup element parser.

from Defaults import *
from DocParser import DocParser
from FunctionParser import FunctionParser

class VarGroupParser:
#{

	##----------------------------------------------------------------------
	def __init__(self, elVarGroup, clpParent):
	#{
		self.elVarGroup = elVarGroup
		self.strStructName = elVarGroup.get("name")
		self.aelMembers = self.elVarGroup.findall('member')
		self.clpParent = clpParent

		self.clpParent.addTypeUsed("VarGroup")
	#}

	##----------------------------------------------------------------------
	def _defaultConstructor(self):
	#{
		""" Returns the default constructor for a var group. """

		astrConstructors = [ 'VarGroup(parent)' ]
		for elMember in self.aelMembers:
			astrConstructors.append("%s(this)" % elMember.get("name"))

		strOut = """\
		%s(TeulTimeMS& parent)
			: %s
			{}
"""
		return strOut % (self.strStructName, "\n\t\t\t, ".join(astrConstructors))
	#}

	##----------------------------------------------------------------------
	def _copyOperator(self):
	#{
		""" Returns copy operator definition. """

		astrCopies = [ 'm_lastModified = rhs.m_lastModified;' ]
		for elMember in self.aelMembers:
			strName = elMember.get("name")
			astrCopies.append("%s.operator=(rhs.%s);" % (strName, strName))

		strOut = """\
		%s& operator = (const %s& rhs)
		{
			%s
			return *this;
		}
"""

		return strOut % (self.strStructName, self.strStructName, "\n\t\t\t".join(astrCopies))
	#}

	##----------------------------------------------------------------------
	def _groupPacker(self):
	#{
		""" Returns the group packer definition. """

		astrMembers = [ ("%s.packer(mode, a, lastPack, changes, fieldNo++);" % elMember.get("name")) for elMember in self.aelMembers ]

		strOut = """\
		virtual void groupPacker(LOGICAL mode, ACTION_OBJECT* const a, const TeulTimeMS& lastPack, UINT32& changes)
		{
			UINT32 fieldNo = 0;
			%s
		}
"""

		return strOut % "\n\t\t\t".join(astrMembers)
	#}

	##----------------------------------------------------------------------
	def _members(self):
	#{
		""" Returns the member declaration list. """

		global astrTypeDefaults

		strOut = ""
		for elMember in self.aelMembers:
			strType = elMember.get("type")
			strDefault = elMember.get("default")

			self.clpParent.addTypeUsed("SyncVar")
			self.clpParent.addTypeUsed(strType)

			strOut += "\t\t//! %s\n" % elMember.get("brief")

			if not strDefault:
				if strType in astrTypeDefaults:
					strDefault = astrTypeDefaults[strType]

			if strDefault:
				strOut += "\t\tWWII::SyncVar<%s,%s> %s;\n" % (strType, strDefault, elMember.get("name"))
			else:
				strOut += "\t\tWWII::SyncVar<%s> %s;\n" % (strType, elMember.get("name"))

		return strOut
	#}

	##----------------------------------------------------------------------
	def Parse(self):
	#{
		strName = self.strStructName
		strMemb = strName.lower()

		dp = DocParser("@struct %s" % self.strStructName, self.elVarGroup.find("doc"), "\t")

		strOut  = dp.Parse()
		strOut += "\tpublic:\n"
		strOut += "\tstruct %s : public WWII::VarGroup\n\t{\n" % strName
		strOut += self._defaultConstructor()
		strOut += self._copyOperator()
		strOut += self._groupPacker()
		strOut += self._members()

		for elFunc in self.elVarGroup.findall("function"):
			fp = FunctionParser(elFunc, "\t\t")
			strOut += "\n"
			strOut += fp.Parse()

		strOut += "\t} m_%s;\n" % strMemb

		strOut += "\tconst %s& %s() const { return m_%s; }\n" % (strName, strMemb, strMemb)
		strOut += "\t%s& %s() { return m_%s; }\n\n" % (strName, strMemb, strMemb)

		return strOut
	#}

#}

