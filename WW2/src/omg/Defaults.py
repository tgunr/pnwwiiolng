#! /usr/bin/env python

astrTypeDefaults = \
{
	'bool'				:	'false'
,	'UINT8'				:	'0'
,	'UINT16'			:	'0'
,	'UINT32'			:	'0'
,	'UINT64'			:	'0ULL'
,	'SINT8'				:	'0'
,	'SINT16'			:	'0'
,	'SINT32'			:	'0'
,	'SINT64'			:	'0LL'
,	'ChatHandle'		:	'0ULL'
,	'instanceid_t'		:	'0ULL'
,	'playerid_t'		:	'0'
,	'sortieid_t'		:	'0'
,	'Teul::NetTimeMS'	:	'0LL'
,	'SCHAR'				:	'0'
,	'UCHAR'				:	'0U'
}

# Standard dependencies.
astrTypeToHeader = \
{
	"FULL_OBJ_ID"			:	"wwii/wwiiFullObjIDPacker.h"
,	"Teul::NetTimeMS"		:	"teul/teulNetTimeMS.h"
,	"WorldPosition_m"		:	"wwii/wwiiServerTrackedObjectPacker.h"
,	"WEAPON_SPEC"			:	"weap/weapSpecPacker.h"
,	"ServerTrackedObject"	:	"wwii/wwiiServerTrackedObjectPacker.h"
,	"WWII::SyncVar"			:	"wwii/wwiiSyncVar.h"
,	"WWII::VarGroup"		:	"wwii/wwiiSyncVar.h"
,	"SyncVar"				:	"wwii/wwiiSyncVar.h"
,	"VarGroup"				:	"wwii/wwiiSyncVar.h"
}

strDefaultCode = """\
	//! User implemented: somewhere to provide a reference to the Teul Callback for this object.
	public: static TEUL_CALLBACK* Callback(void) ;
"""

strDefaultVarGroupCode = """\
	//! Last modified time of the structure or a member var group.
	protected: TeulTimeMS m_lastModified ;

	//! Get milisecond timestamp of last modification.
	//! @return Local teulClock64 timestamp of the last modification of a managed member of the structure.
	public: const TeulTimeMS& lastModified() const { return m_lastModified ; }

	//! How many bits required to describe number of groups changed.
	public: UINT32 groupsChangedBits() const { return %u ; }
"""

strDefaultPackerComment = """\
	///! User implemented: Routine for serializing/deserializing this data.
"""

strDefaultPacker = """\
	public: bool packer(LOGICAL mode, ACTION_OBJECT* a, SINT32 size) ;
"""

