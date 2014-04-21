#pragma once
// Created: Wed Nov  1 15:19:42 CST 2006
//
// A full object ID describes a terrain object in
// detail; because of the way this information is
// compacted for transmission, much of the detail
// has been rendered obsolete.
// Functions are provided for converting FULL_OBJ_IDs
// to 48 bits of a UINT64 (key()) and back (fromKey()).
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//tolua_begin

// Class of strat element
enum STRAT_CLASS
{
	CLASS_CP=1,
	CLASS_FACILITY,
	CLASS_BUILDING,
	CLASS_LINK,
	CLASS_WAYPOINT,
	// Add new classes here
	MAX_STRAT_CLASSES
};

struct OCTET_CELL
{
	SINT16	ox;
	SINT16	oy;
};

// remember you may cast this to a 64 bit in ID;
struct FULL_OBJ_ID
{
	OCTET_CELL	cell;				// octet (get supercell by >> 4)
	UINT16		id;					// id within the supercell
	SINT8		objClass;			// class is STRAT_CLASS
	SINT8		objType;			// type

//tolua_end
	static const UINT32 PrintLen = 128;	// Length of print buffer size.

	void populate(SINT32 sx, SINT32 sy, struct REL_OBJ_ID roid);

	//// NOTE ABOUT CONSTRUCTORS:
	// Due to some legacy code, you cannot give FULL_OBJ_ID a constructor
	// or destructor.
	template<typename _Type>
	void Populate(const _Type* source, UINT16 withId, SINT8 withClass, SINT8 withType)
	{
		cell.ox = source->posx / 800;
		cell.oy = source->posy / 800;
		this->id = withId;
		objClass = withClass;
		objType = withType;
	}

	void FromKey(const UINT64& key)
	{
		cell.ox = (SINT16)((key >> 28) << 4);
		cell.oy = (SINT16)((key >> 16) << 4);
		this->id = (UINT16)(key & 0xffff);
		objClass = 0;
		objType = 0;
	}
//tolua_begin

	// The cell OX and OY aren't precise and don't need to be,
	// so we need to eliminate the lower 4 bits in the comparison
	bool sameCell(SINT16 x, SINT16 y) const { return (cell.ox>>4 == x>>4 && cell.oy>>4 == y>>4); }
	bool sameCell(const OCTET_CELL& rhs) const { return sameCell(rhs.ox, rhs.oy); }
	bool sameCell(const FULL_OBJ_ID& rhs) const { return sameCell(rhs.cell.ox, rhs.cell.oy); }

	bool operator == (const FULL_OBJ_ID& rhs) const { return id == rhs.id && sameCell(rhs); }
//tolua_end
	bool operator != (const FULL_OBJ_ID& rhs) const { return id != rhs.id || !sameCell(rhs); }
//tolua_begin

	// Helper function for converting a fullObjID into a UINT64
	UINT64 Key() const
	{
		static const UINT64 SC_MASK = (0x0fff);		// 12 bits of supercell id
		return (((UINT64)(cell.ox >> 4) & SC_MASK) << 28)	// 12 bits of ox
			|  (((UINT64)(cell.oy >> 4) & SC_MASK) << 16)	// 12 bits of oy
			|  (id & 0xffff)								// 16 bits of id
			;
	}

//tolua_end
	void pack(struct ACTION_OBJECT* const a, SINT32 bits = 0) const;
	void unpack(struct ACTION_OBJECT* const a, SINT32 bits = 0);
//tolua_begin
};

extern const FULL_OBJ_ID nullObjID;

//tolua_end
#define PRF_FULL_OBJ_ID "%d.%d:%u"
#define PRA_FULL_OBJ_ID(s) (s).cell.ox, (s).cell.oy, (s).id
//tolua_begin

static inline const char* weapPrintFullObjIDC(const FULL_OBJ_ID* id, char* buffer)
{
	if ( id == nullptr )
		return "<nullFOID>";

	sprintf(buffer, "x:%d y:%d id:%hu c:%hd t:%hd"
				, id->cell.ox
				, id->cell.oy
				, (unsigned short)id->id
				, (short)id->objClass
				, (short)id->objType);

	return buffer;
}
//tolua_end
