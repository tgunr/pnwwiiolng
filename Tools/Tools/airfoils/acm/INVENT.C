/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991-1998  Riley Rainey
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 dated June, 1991.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program;  if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave., Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <pm.h>
#if defined(HAVE_STDLIB_H)
#include <stdlib.h>
#else
#include <malloc.h>
#endif

struct lex_record;

int       ParseAircraft(struct lex_record *p, craftType ** c1), ParseValue(struct lex_record *p),
          ParseTable(struct lex_record *p), ParsePoint(struct lex_record *p);
int       ParseStation(struct lex_record *p);
int       ParseDISEntityType(struct lex_record *p);
int       ParseEntityMapEntry(struct lex_record *p, entity_object_map *po);
int       ParseMunitionMapEntry(struct lex_record *p, munition_map *po);

static craftType c;

extern craftType *newCraft(void);
extern ITable * copyITable (ITable *oldp);

extern double genericThrust PARAMS((craft *));
extern void genericResupply PARAMS((craft *));

typedef enum {
	RESERVED_WORD,
	vtDOUBLE,
	vtLONG,
	vtANGLE,
	vtNMILES,
	vtKNOTS,
	vtTABLE,
	vtPOINT,
	vtSTRING,
	vtSTATION,
	vtENTITY
} value_type;

typedef enum {
	Nil,
	EndOfFile,

/*
 *  Reserved words must be added to this section
 */

	TOKEN_NUMBER,
	RW_PUNCTUATION,
	TOKEN_STRING,
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	TOKEN_COMMA,
	TOKEN_COLON,
	TOKEN_SEMICOLON,
	RW_USE,
	RW_AIRCRAFT,
	RW_DESCRIPTION,
	RW_HARDPOINT,
	RW_KINETIC,
	RW_EXPLOSIVE,
	RW_M61A1,
	RW_AIM9M,
	RW_MK82,

/*
 *  Fields in the craftType structure must be added in this section
 */

	Object,
	InternalModel,
	AspectRatio,
	CLPosStall,
	CLNegStall,
	BetaStall,
	CLift,
	CDb,
	CnBeta,
	ClBeta,
	CDBOrigin,
	CDBFactor,
	CDBPhase,
	CYbeta,
	Clda,
	Cldr,
	Clp,
	Cmq,
	Cnr,
	maxAileron,
	maxRudder,
	effElevator,
	effRudder,
	SeTrimTakeoff,
	Ixx,
	Iyy,
	Izz,
	CmAlpha,
	CmFactor,
	MaxFlap,
	CFlap,
	FlapDrag,
	FlapRate,
	CGearDrag,
	GearRate,
	MaxSpeedBrake,
	CSpeedBrake,
	SpeedBrakeRate,
	SpeedBrakeIncr,
	WingArea,
	WingSpan,
	WingChord,
	EmptyWeight,
	MaxFuel,
	MaxThrust,
	MaxABThrust,
	Thrust,
	ABThrust,
	EngineLag,
	SpFuelConsump,
	SpABFuelConsump,
	GroundingPoint,
	ViewPoint,
	MuStatic,
	MuKinetic,
	MuBStatic,
	MuBKinetic,
	MaxNWDef,
	NWincr,
	MaxNWS,
	Rm,
	Rn,
	Dm,
	Dn,
	Km,
	Kn,
	Gm,
	Gn,
	CmMax,
	CnMax,
	TailExtent,
	StructurePts,
	RadarOutput,
	RadarTRange,
	RadarDRange,
	TEWSThreshold,
	HardPoint0,
	HardPoint1,
	HardPoint2,
	HardPoint3,
	HardPoint4,
	HardPoint5,
	HardPoint6,
	HardPoint7,
	HardPoint8,
	WeaponStation,
	WeaponCount,
	DISEntityType,
	Description
} field_id;

struct keyword_info {
	char     *word;
	value_type type;
	field_id  id;
	char     *ptr;
};

#define A(x)	(char *) x

struct keyword_info keywords[] =
{

	{"Description", vtSTRING, Description, A(&c.description)},
	{"Object", vtSTRING, Object, A(&c.objname)},
	{"InternalModel", vtSTRING, InternalModel, A(&c.modelname)},
	{"AspectRatio", vtDOUBLE, AspectRatio, A(&c.aspectRatio)},
	{"CLPosStall", vtANGLE, CLPosStall, A(&c.CLPosStall)},
	{"CLNegStall", vtANGLE, CLNegStall, A(&c.CLNegStall)},
	{"BetaStall", vtANGLE, BetaStall, A(&c.betaStall)},
	{"CLift", vtTABLE, CLift, A(&c.CLift)},
	{"CDb", vtTABLE, CDb, A(&c.CDb)},
	{"CnBeta", vtTABLE, CnBeta, A(&c.CnBeta)},
	{"ClBeta", vtTABLE, ClBeta, A(&c.ClBeta)},
	{"CDBOrigin", vtDOUBLE, CDBOrigin, A(&c.CDBOrigin)},
	{"CDBFactor", vtDOUBLE, CDBFactor, A(&c.CDBFactor)},
	{"CDBPhase", vtANGLE, CDBPhase, A(&c.CDBPhase)},
	{"CYBeta", vtDOUBLE, CYbeta, A(&c.CYbeta)},
	{"Clda", vtDOUBLE, Clda, A(&c.Clda)},
	{"Cldr", vtDOUBLE, Cldr, A(&c.Cldr)},
	{"Clp", vtDOUBLE, Clp, A(&c.Clp)},
	{"Cmq", vtDOUBLE, Cmq, A(&c.Cmq)},
	{"Cnr", vtDOUBLE, Cnr, A(&c.Cnr)},
	{"MaxAileron", vtANGLE, maxAileron, A(&c.maxAileron)},
	{"MaxRudder", vtANGLE, maxRudder, A(&c.maxRudder)},
	{"EffElevator", vtDOUBLE, effElevator, A(&c.effElevator)},
	{"EffRudder", vtDOUBLE, effRudder, A(&c.effRudder)},
	{"SeTrimTakeoff", vtANGLE, SeTrimTakeoff, A(&c.SeTrimTakeoff)},
	{"Ixx", vtDOUBLE, Ixx, A(&c.I.m[0][0])},
	{"Iyy", vtDOUBLE, Iyy, A(&c.I.m[1][1])},
	{"Izz", vtDOUBLE, Izz, A(&c.I.m[2][2])},
	{"CmAlpha", vtDOUBLE, CmAlpha, A(&c.cmSlope)},
	{"CmFactor", vtDOUBLE, CmFactor, A(&c.cmFactor)},
	{"MaxFlap", vtANGLE, MaxFlap, A(&c.maxFlap)},
	{"CFlap", vtDOUBLE, CFlap, A(&c.cFlap)},
	{"CFlapDrag", vtDOUBLE, FlapDrag, A(&c.cFlapDrag)},
	{"FlapRate", vtANGLE, FlapRate, A(&c.flapRate)},
	{"CGearDrag", vtDOUBLE, CGearDrag, A(&c.cGearDrag)},
	{"GearRate", vtANGLE, GearRate, A(&c.gearRate)},
	{"MaxSpeedBrake", vtANGLE, MaxSpeedBrake, A(&c.maxSpeedBrake)},
	{"CSpeedBrake", vtDOUBLE, CSpeedBrake, A(&c.cSpeedBrake)},
	{"SpeedBrakeRate", vtANGLE, SpeedBrakeRate, A(&c.speedBrakeRate)},
	{"SpeedBrakeIncr", vtANGLE, SpeedBrakeIncr, A(&c.speedBrakeIncr)},
	{"WingArea", vtDOUBLE, WingArea, A(&c.wingS)},
	{"WingHalfSpan", vtDOUBLE, WingSpan, A(&c.wings)},
	{"Chord", vtDOUBLE, WingChord, A(&c.c)},
	{"EmptyWeight", vtDOUBLE, EmptyWeight, A(&c.emptyWeight)},
	{"MaxFuel", vtDOUBLE, MaxFuel, A(&c.maxFuel)},
	{"MaxThrust", vtDOUBLE, MaxThrust, A(&c.maxThrust)},
	{"MaxABThrust", vtDOUBLE, MaxABThrust, A(&c.maxABThrust)},
	{"Thrust", vtTABLE, Thrust, A(&c.Thrust)},
	{"ABThrust", vtTABLE, ABThrust, A(&c.ABThrust)},
	{"EngineLag", vtDOUBLE, EngineLag, A(&c.engineLag)},
	{"SpFuelConsump", vtDOUBLE, SpFuelConsump, A(&c.spFuelConsump)},
	{"SpABFuelConsump", vtDOUBLE, SpABFuelConsump, A(&c.spABFuelConsump)},
	{"GroundingPoint", vtPOINT, GroundingPoint, A(&c.groundingPoint)},
	{"ViewPoint", vtPOINT, ViewPoint, A(&c.viewPoint)},
	{"MuStatic", vtDOUBLE, MuStatic, A(&c.muStatic)},
	{"MuKinetic", vtDOUBLE, MuKinetic, A(&c.muKinetic)},
	{"MuBStatic", vtDOUBLE, MuBStatic, A(&c.muBStatic)},
	{"MuBKinetic", vtDOUBLE, MuBKinetic, A(&c.muBKinetic)},
	{"MaxNWDef", vtANGLE, MaxNWDef, A(&c.maxNWDef)},
	{"NWIncr", vtANGLE, NWincr, A(&c.NWIncr)},
	{"MaxNWS", vtKNOTS, MaxNWS, A(&c.maxNWS)},
	{"Rm", vtPOINT, Rm, A(&c.rm)},
	{"Rn", vtPOINT, Rn, A(&c.rn)},
	{"Dm", vtDOUBLE, Dm, A(&c.Dm)},
	{"Dn", vtDOUBLE, Dn, A(&c.Dn)},
	{"Km", vtDOUBLE, Km, A(&c.Km)},
	{"Kn", vtDOUBLE, Kn, A(&c.Kn)},
	{"Gm", vtDOUBLE, Gm, A(&c.Gm)},
	{"Gn", vtDOUBLE, Gn, A(&c.Gn)},
	{"CmMax", vtDOUBLE, CmMax, A(&c.cmMax)},
	{"CnMax", vtDOUBLE, CnMax, A(&c.cnMax)},
	{"TailExtent", vtPOINT, TailExtent, A(&c.tailExtent)},
	{"StructurePoints", vtLONG, StructurePts, A(&c.structurePts)},
	{"RadarOutput", vtDOUBLE, RadarOutput, A(&c.radarOutput)},
	{"RadarTRange", vtNMILES, RadarTRange, A(&c.radarTRange)},
	{"RadarDRange", vtNMILES, RadarDRange, A(&c.radarDRange)},
	{"TEWSThreshold", vtDOUBLE, TEWSThreshold, A(&c.TEWSThreshold)},
	{"HardPoint1", vtPOINT, HardPoint1, A(&c.wStation[1])},
	{"HardPoint2", vtPOINT, HardPoint2, A(&c.wStation[2])},
	{"HardPoint3", vtPOINT, HardPoint3, A(&c.wStation[3])},
	{"HardPoint4", vtPOINT, HardPoint4, A(&c.wStation[4])},
	{"HardPoint5", vtPOINT, HardPoint5, A(&c.wStation[5])},
	{"HardPoint6", vtPOINT, HardPoint6, A(&c.wStation[6])},
	{"HardPoint7", vtPOINT, HardPoint7, A(&c.wStation[7])},
	{"HardPoint8", vtPOINT, HardPoint8, A(&c.wStation[8])},
	{"HardPoint0", vtPOINT, HardPoint0, A(&c.wStation[0])},
	{"WeaponCount", vtLONG, WeaponCount, A(&c.sCount)},
	{"WeaponStation", vtSTATION, WeaponStation, 0},

	{"use", RESERVED_WORD, RW_USE, 0},
	{"aircraft", RESERVED_WORD, RW_AIRCRAFT, 0},
	{"description", RESERVED_WORD, RW_DESCRIPTION, 0},
	{"hardpoint", RESERVED_WORD, RW_HARDPOINT, 0},
    {"kinetic", RESERVED_WORD, RW_KINETIC, 0},
	{"explosive", RESERVED_WORD, RW_EXPLOSIVE, 0},
	{"blast", RESERVED_WORD, RW_EXPLOSIVE, 0},

	{"M61A1", RESERVED_WORD, RW_M61A1, 0},
	{"AIM9M", RESERVED_WORD, RW_AIM9M, 0},
	{"MK82",  RESERVED_WORD, RW_MK82,  0},

	{"DISEntityType", vtENTITY, DISEntityType, A(&c.entityType)},
	{"DISAltEntityType", vtENTITY, DISEntityType, A(&c.altEntityType)},

	{NULL, RESERVED_WORD, Nil, 0}
};

typedef union {
	struct keyword_info *kw;
	double    double_value;
	ITable   *table_value;
	char     *string_value;
	long      long_value;
} lex_val;

static lex_val lex_value;

struct lex_record {
	char     *filename;
	FILE     *f;
	int       lineno;
	int       lookahead_valid;
	int       lookahead;
	int       stack_top;
	lex_val   value_stack[16];
};

field_id NextToken(struct lex_record * p);
void Resync(struct lex_record *p, field_id token);

#define push_value(p, type, val) \
	p->value_stack[p->stack_top++].type = val

#define pop_value(p, type) (p->value_stack[--p->stack_top].type)

#define input(p)	(p->lookahead_valid \
				? (p->lookahead_valid = 0, p->lookahead) \
				: (((p->lookahead = getc(p->f)) == '\n') \
					? (p->lineno++, p->lookahead) \
					: p->lookahead))

#define unput(p, c)	{ p->lookahead = c; p->lookahead_valid = 1; }

#define InitializeLexRecord(p)	{ p->lookahead_valid = 0; }

static char token[256];
static int token_length = 0;

#define STATE_INITIAL	0
#define STATE_WORD	    1
#define STATE_NUMBER	2
#define STATE_STRING	3
#define STATE_COMMENT	4

extern FILE *acm_fopen(char *name, char *access);

struct lex_record *
OpenSourceFile(char *name)
{
	struct lex_record *p;
	FILE     *f;

	if ((f = acm_fopen(name, "r")) == (FILE *) NULL) {
		return (struct lex_record *) NULL;
	}

	p = (struct lex_record *) malloc(sizeof(struct lex_record));

	p->filename = strdup(name);
	p->lineno = 1;
	p->lookahead_valid = 0;
	p->stack_top = 0;
	p->f = f;

	return p;
}

int
compileAircraftInventory(void)
{
	struct lex_record *p;
	craftType *c, *c1;
	int       code = 0;
	FILE     *f;

	if ((p = OpenSourceFile("inventory")) == NULL) {
		fprintf(stderr, "unable to open aircraft inventory file\n");
		return -1;
	}

	while (1) {
		if (ParseAircraft(p, &c) == 0) {
			if (c) {
				c1 = newCraft();
				*c1 = *c;
				free((char *) c);

/*
 *  Initialize some other interesting values
 */

				f = acm_fopen(c1->objname, "r");
				c1->object = VReadDepthCueuedObject(f, 1);
				fclose(f);
				c1->placeProc = NULL;
				c1->damageBits = SYS_NODAMAGE;
				c1->damageBits &= ~SYS_ENGINE2;
				c1->thrust = genericThrust;
				c1->resupply = genericResupply;

/*
 *  Some older values are now derived from more precise information
 */

				c1->gearD1 = c1->rn.x - c1->rm.x;
				c1->gearD2 = c1->rm.x;
				c1->CLOrigin = interpolate(c1->CLift, 0.0);
				c1->CLSlope = (interpolate(c1->CLift, DEGtoRAD(10.0))
							   - c1->CLOrigin) / DEGtoRAD(10.0);
			}
			else {
				break;
			}
		}
		else {
			code = -1;
			break;
		}
	}

	fclose(p->f);
	free(p->filename);
	free((char *) p);
	return code;
}

int
compileEntityMap(char *name, int *count, entity_object_map **pmap)
{
	struct lex_record *p;
	int       code = 0;
	entity_object_map po;
	int n = 0;

	*pmap = NULL;
	*count = 0;

	if ((p = OpenSourceFile(name)) == NULL) {
		fprintf(stderr, "unable to open entity map file\n");
		return -1;
	}

	while (ParseEntityMapEntry(p, &po) == 0 && code == 0) {
		*pmap = realloc (*pmap, sizeof(entity_object_map) * (n+1) );
		if (!pmap) {
			code = -1;
		}
		(*pmap)[n] = po;
		n++;
	}

	*count = n;

	fclose(p->f);
	free(p->filename);
	free((char *) p);
	return code;
}

int
compileMunitionMap(char *name, int *count, munition_map **pmap)
{
	struct lex_record *p;
	int       code = 0;
	munition_map po;
	int n = 0;

	*pmap = NULL;
	*count = 0;

	if ((p = OpenSourceFile(name)) == NULL) {
		fprintf(stderr, "unable to open entity map file\n");
		return -1;
	}

	while (ParseMunitionMapEntry(p, &po) == 0 && code == 0) {
		*pmap = realloc (*pmap, sizeof(munition_map) * (n+1) );
		if (!pmap) {
			code = -1;
		}
		(*pmap)[n] = po;
		n++;
	}

	*count = n;

	fclose(p->f);
	free(p->filename);
	free((char *) p);
	return code;
}

int
ParseEntityMapEntry(struct lex_record *p, entity_object_map *po)
{
	long ival, result;

	memset (po, 0, sizeof(entity_object_map));

	result = ParseDISEntityType(p);

	/* end-of-file */
	if (result == -2) {
		return -1;
	}

	/* other error */
	if (result != 0) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.extra = 0;
	}
	else {
		po->entity_mask.extra = 1;
		po->entity_type.extra = (unsigned char) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.specific = 0;
	}
	else {
		po->entity_mask.specific = 1;
		po->entity_type.specific = (unsigned char) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.subcategory = 0;
	}
	else {
		po->entity_mask.subcategory = 1;
		po->entity_type.subcategory = (unsigned char) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.category = 0;
	}
	else {
		po->entity_mask.category = 1;
		po->entity_type.category = (unsigned char) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.country = 0;
	}
	else {
		po->entity_mask.country = 1;
		po->entity_type.country = (unsigned short) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.domain = 0;
	}
	else {
		po->entity_mask.domain = 1;
		po->entity_type.domain = (unsigned char) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.kind = 0;
	}
	else {
		po->entity_mask.kind = 1;
		po->entity_type.kind = (unsigned char) ival;
	}

	if (NextToken(p) != TOKEN_COMMA) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	if (NextToken(p) != TOKEN_STRING) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	po->object_name = strdup( token );

	if (NextToken(p) != TOKEN_SEMICOLON) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	return 0;
}

/*
 *  munition_entity_type , warhead_type , explosion_diameter , damage_factor ;
 */

int
ParseMunitionMapEntry(struct lex_record *p, munition_map *po)
{
	long ival, result;
	field_id t;

	memset (po, 0, sizeof(munition_map));

	result = ParseDISEntityType(p);

	/* end-of-file */
	if (result == -2) {
		return -1;
	}

	/* other error */
	if (result != 0) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.extra = 0;
	}
	else {
		po->entity_mask.extra = 1;
		po->entity_type.extra = (unsigned char) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.specific = 0;
	}
	else {
		po->entity_mask.specific = 1;
		po->entity_type.specific = (unsigned char) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.subcategory = 0;
	}
	else {
		po->entity_mask.subcategory = 1;
		po->entity_type.subcategory = (unsigned char) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.category = 0;
	}
	else {
		po->entity_mask.category = 1;
		po->entity_type.category = (unsigned char) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.country = 0;
	}
	else {
		po->entity_mask.country = 1;
		po->entity_type.country = (unsigned short) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.domain = 0;
	}
	else {
		po->entity_mask.domain = 1;
		po->entity_type.domain = (unsigned char) ival;
	}

	ival = pop_value(p, long_value);
	if (ival == -1) {
		po->entity_mask.kind = 0;
	}
	else {
		po->entity_mask.kind = 1;
		po->entity_type.kind = (unsigned char) ival;
	}

	if (NextToken(p) != TOKEN_COMMA) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	if (NextToken(p) != TOKEN_NUMBER) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	/* warhead type */

	if (lex_value.double_value >= 0) {
		ival = (int) (lex_value.double_value + 0.5);
	}
	else {
		ival = (long)(lex_value.double_value - 0.5);
	}

	if (ival == -1) {
		po->warhead_mask = 0;
	}
	else {
		po->warhead_mask = 1;
		po->warhead_type = (unsigned short) ival;
	}

	if (NextToken(p) != TOKEN_COMMA) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	/* explosion diameter */

	if (NextToken(p) != TOKEN_NUMBER) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	po->explosion_diameter_meters = FEETtoMETERS(lex_value.double_value);

	if (NextToken(p) != TOKEN_COMMA) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	/* damage factor */

	if (NextToken(p) != TOKEN_NUMBER) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	po->damage_factor = lex_value.double_value;

	if (NextToken(p) != TOKEN_COMMA) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	/* warhead class: kinetic or explosive */

	t = NextToken(p);

	if (t != RW_KINETIC && t != RW_EXPLOSIVE) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	po->kinetic_flag = (t == RW_KINETIC) ? 1 : 0;

	if (NextToken(p) != TOKEN_SEMICOLON) {
		Resync(p, TOKEN_SEMICOLON);
		return -1;
	}

	return 0;
}

void
ParseError(struct lex_record *p, char *s)
{
	fprintf(stderr, "error in file %s (line %d):\n\t%s\n", p->filename,
			p->lineno, s);
}

field_id
NextTokenx(struct lex_record *p)
{
	register int c, state = STATE_INITIAL;
	register struct keyword_info *q;

	token_length = 0;

	while ((c = input(p)) != EOF) {

		switch (state) {

		case STATE_INITIAL:

			if (isalpha(c)) {
				token[token_length++] = c;
				state = STATE_WORD;
			}
			else if (isspace(c)) {
				continue;
			}
			else if (isdigit(c) || c == '-' || c == '+' || c == '.') {
				token[token_length++] = c;
				state = STATE_NUMBER;
			}
			else if (c == '"') {
				state = STATE_STRING;
			}
			else if (c == '#') {
				state = STATE_COMMENT;
			}
			else {
				token[0] = c;
				token[1] = '\0';
#ifdef DEBUG
				printf("other %s\n", token);
#endif
				switch (c) {
				case ',':
					return TOKEN_COMMA;
				case ':':
					return TOKEN_COLON;
				case ';':
					return TOKEN_SEMICOLON;
				case '{':
					return TOKEN_LEFT_BRACE;
				case '}':
					return TOKEN_RIGHT_BRACE;
				default:
					ParseError(p, "invalid character");
					state = STATE_INITIAL;
				}
			}
			break;

		case STATE_WORD:
		case STATE_NUMBER:
			if (isspace(c) || c == ':' || c == ',' || c == '{' || c == '}') {
				token[token_length] = '\0';
				unput(p, c);
				if (state == STATE_WORD) {
					for (q = keywords; q->word; ++q) {
						if (strcmp(q->word, token) == 0) {
							lex_value.kw = q;
							return q->id;
						}
					}
					return Nil;
				}
				else {
					errno = 0;
					lex_value.double_value = strtod( token, NULL );
					if (errno == ERANGE) {
						printf ("invalid numeric constant: %s\n", token);
					}
					return TOKEN_NUMBER;
				}
			}
			else {
				token[token_length++] = c;
			}
			break;

		case STATE_STRING:

			switch (c) {

			case '"':
				token[token_length] = '\0';
				return TOKEN_STRING;

			case '\n':
				ParseError(p, "strings cannot span a line");
				unput(p, c);
				state = STATE_INITIAL;
				break;

			case '\\':

				switch (c = input(p)) {

				case EOF:
					ParseError(p, "Premature End-of-file");
					break;

				case 'n':
					token[token_length++] = '\n';
					break;

				case 't':
					token[token_length++] = '\t';
					break;

				default:
					token[token_length++] = c;
					break;
				}

			default:
				token[token_length++] = c;
				break;
			}
			break;

		case STATE_COMMENT:
			while (c != EOF) {
				if (c == '\n')
					break;
				c = input(p);
			}
			state = STATE_INITIAL;
			break;

		}
	}

	return EndOfFile;
}

field_id
NextToken(struct lex_record * p)
{
	field_id  t;

	t = NextTokenx(p);

#ifdef DEBUG
	printf("token %s\n", token);
#endif
	return t;
}

/*
 *  Skip to the specified token, if token is Nil, then skip to the end of the
 *  current line.
 */

void
Resync(struct lex_record *p, field_id token)
{
	field_id  t;
	int       c;

	if (token == Nil) {
		while ((c = input(p)) != EOF) {
			if (c == '\n')
				break;
		}
	}
	else {
		while ((t = NextToken(p)) != EndOfFile) {
			if (t == token)
				break;
		}
	}

}

int
ParseAircraft(struct lex_record *p, craftType ** c1)
{
	field_id  t;
	long      n, i;
	double    d;
	VPoint    pt;
	ITable   *table;
	dis_entity_type entity;
	char      s[256];
	struct keyword_info *kw;
	craftType *used;

	memset(&c, 0, sizeof(c));
	*c1 = NULL;

	if ((t = NextToken(p)) != RW_AIRCRAFT) {
		if (t == EndOfFile) {
			return 0;
		}
		else {
			return -1;
		}
	}

	if (NextToken(p) != TOKEN_STRING) {
		return -1;
	}

	c.name = strdup(token);

	if (NextToken(p) != TOKEN_LEFT_BRACE) {
		return -1;
	}

	while ((t = NextToken(p)) != EndOfFile) {

		if (t >= Object || t == RW_USE) {

			kw = lex_value.kw;

			switch (kw->type) {

			case RESERVED_WORD:

				/*
				 *  use "aircraft-type-name"
				 *
				 *  uses a previously defined aircraft as the starting point for defining another
				 */

				if (kw->id == RW_USE) {
					if ( NextToken(p) == TOKEN_STRING ) {
						char * ptmp = c.name;

						n = 1;
						used = lookupCraft( token );
						if (used) {
							c = *used;
							c.CLift    = copyITable (used->CLift   );
							c.CDb      = copyITable (used->CDb     );
							c.CnBeta   = copyITable (used->CnBeta  );
							c.ClBeta   = copyITable (used->ClBeta  );
							c.Thrust   = copyITable (used->Thrust  );
							c.ABThrust = copyITable (used->ABThrust);
							//if (used->name) {
							//	c.name = strdup( used->name );
							//}
							c.name = ptmp;
							if (used->description) {
								c.description = strdup( used->description );
							}
							if (used->modelname) {
								c.modelname = strdup( used->modelname );
							}
							if (used->objname) {
								c.objname = strdup( used->objname );
							}
							for (i=0; i<STATIONS; ++i) {
								if ( used->station[i].type ) {
									c.station[i].type = strdup( used->station[i].type) ;
								}
							}


							n = 0;

						}
						else {
							sprintf(s, "\"%s\" is not a valid aircraft type", token);
							ParseError(p, s);
						}
					}
				}
				break;

			case vtSTRING:
				if (NextToken(p) == TOKEN_STRING) {
					n = 0;
					*((char **) kw->ptr) = strdup(token);
				}
				else
					n = -1;
				break;

			case vtDOUBLE:
			case vtNMILES:
			case vtKNOTS:
				if ((n = ParseValue(p)) == 0) {
					d = pop_value(p, double_value);
					if (kw->type == vtNMILES)
						d *= NM;
					else if (kw->type == vtKNOTS)
						d *= NM / 3600;
					*((double *) kw->ptr) = d;
				}
				break;

			case vtANGLE:
				if ((n = ParseValue(p)) == 0) {
					d = DEGtoRAD(pop_value(p, double_value));
					*((double *) kw->ptr) = d;
				}
				break;

			case vtLONG:
				if ((n = ParseValue(p)) == 0) {
					d = pop_value(p, double_value);
					*((long *) kw->ptr) = (long) (d + 0.5);
				}
				break;

			case vtTABLE:
				if ((n = ParseTable(p)) == 0) {
					table = pop_value(p, table_value);
					*((ITable **) kw->ptr) = table;
				}
				break;

			case vtPOINT:
				if ((n = ParsePoint(p)) == 0) {
					pt.z = pop_value(p, double_value);
					pt.y = pop_value(p, double_value);
					pt.x = pop_value(p, double_value);
					*((VPoint *) kw->ptr) = pt;
				}
				break;

			case vtSTATION:
				if ((n = ParseStation(p)) == 0) {
					i = pop_value(p, long_value);
					c.station[i].type =
						pop_value(p, string_value);
					c.station[i].info =
						pop_value(p, long_value);
					c.station[i].info2 =
						pop_value(p, long_value);
					c.station[i].info3 =
						pop_value(p, long_value);
				}
				break;

			case vtENTITY:
				if ((n = ParseDISEntityType(p)) == 0) {
					entity.extra = (unsigned char)
						pop_value(p, long_value);
					entity.specific = (unsigned char)
						pop_value(p, long_value);
					entity.subcategory = (unsigned char)
						pop_value(p, long_value);
					entity.category = (unsigned char)
						pop_value(p, long_value);
					entity.country = (unsigned char)
						pop_value(p, long_value);
					entity.domain = (unsigned char)
						pop_value(p, long_value);
					entity.kind = (unsigned char)
						pop_value(p, long_value);
					*((dis_entity_type *) kw->ptr) = entity;
				}
				break;
			}

			if (n != 0) {
				sprintf(s, "invalid syntax for %s\
 parameter", kw->word);
				ParseError(p, s);
			}
		}
		else if (t == TOKEN_RIGHT_BRACE) {
			*c1 = (craftType *) malloc(sizeof(craftType));
			**c1 = c;
			return 0;
		}
		else {
			sprintf(s, "\"%s\" was found where another token was\
 expected", token);
			ParseError(p, s);
			return -1;
		}
	}

	return -1;
}

/*
 *  Parse syntax:  '{' number ',' number ',' number '}'
 */

int
ParsePoint(struct lex_record *p)
{

	if (NextToken(p) != TOKEN_LEFT_BRACE) {
		Resync(p, TOKEN_RIGHT_BRACE);
		return -1;
	}

	if (NextToken(p) != TOKEN_NUMBER) {
		Resync(p, TOKEN_RIGHT_BRACE);
		return -1;
	}

	push_value(p, double_value, lex_value.double_value);

	if (NextToken(p) != TOKEN_COMMA) {
		Resync(p, TOKEN_RIGHT_BRACE);
		return -1;
	}

	if (NextToken(p) != TOKEN_NUMBER) {
		Resync(p, TOKEN_RIGHT_BRACE);
		return -1;
	}

	push_value(p, double_value, lex_value.double_value);

	if (NextToken(p) != TOKEN_COMMA) {
		Resync(p, TOKEN_RIGHT_BRACE);
		return -1;
	}

	if (NextToken(p) != TOKEN_NUMBER) {
		Resync(p, TOKEN_RIGHT_BRACE);
		return -1;
	}

	push_value(p, double_value, lex_value.double_value);

	if (NextToken(p) != TOKEN_RIGHT_BRACE) {
		Resync(p, TOKEN_RIGHT_BRACE);
		return -1;
	}

	return 0;
}

/*
 *  Parse syntax:  '{' number_list '}'
 *
 *  Where number_list is a collection of zero or more comma separated
 *  numbers.  The list of numbers must be an even count.
 *
 */

int
ParseTable(struct lex_record *p)
{
	field_id  t;
	double    x[64], y[64];
	int       count = 0, i;
	ITable   *table;
	IEntry   *entry;

	if (NextToken(p) != TOKEN_LEFT_BRACE) {
		Resync(p, TOKEN_RIGHT_BRACE);
		return -1;
	}

	while ((t = NextToken(p)) != TOKEN_RIGHT_BRACE) {

		if (t == EndOfFile)
			return -1;

		if (t == TOKEN_NUMBER) {

			if (count == 64) {
				ParseError(p, "too many table entries");
				return -1;
			}

			x[count] = lex_value.double_value;

			if (NextToken(p) != TOKEN_COMMA) {
				Resync(p, TOKEN_RIGHT_BRACE);
				return -1;
			}

			if (NextToken(p) != TOKEN_NUMBER) {
				Resync(p, TOKEN_RIGHT_BRACE);
				return -1;
			}
			y[count++] = lex_value.double_value;

			t = NextToken(p);

			if (t == TOKEN_RIGHT_BRACE)
				goto done;
			else if (t != TOKEN_COMMA) {
				Resync(p, TOKEN_RIGHT_BRACE);
				return -1;
			}
		}
		else {
			Resync(p, TOKEN_RIGHT_BRACE);
			return -1;
		}
	}

  done:

/*
 *  Build an interpolation table
 */

	table = (ITable *) malloc(sizeof(ITable));
	entry = (IEntry *) malloc(sizeof(IEntry) * (count - 1));
	table->minX = (float_t) x[0];
	table->count = count - 1;
	table->entry = entry;

	for (i = 1; i < count; ++i) {
		entry[i - 1].x = (float_t) x[i];
		entry[i - 1].m = (float_t) ((y[i] - y[i - 1]) / (x[i] - x[i - 1]));
		entry[i - 1].b = (float_t) (y[i] - (x[i] * entry[i - 1].m));
	}

	push_value(p, table_value, table);

	return 0;
}

int
ParseValue(struct lex_record *p)
{
	if (NextToken(p) != TOKEN_NUMBER) {
		Resync(p, Nil);
		return -1;
	}
	push_value(p, double_value, lex_value.double_value);
	return 0;
}

int
ParseWeaponType(struct lex_record *p)
{
	field_id t;
	int result = 0;

	t = NextToken(p);

	if (t == RW_M61A1) {
		push_value( p, long_value, WK_M61A1 );
	}
	else if (t == RW_AIM9M) {
		push_value( p, long_value, WK_AIM9M );
	}
	else if (t == RW_MK82) {
		push_value( p, long_value, WK_MK82 );
	}
	else {
		Resync(p, Nil);
		result = -1;
	}

	return 0;
}

/*
 *  Parse syntax:  number weapon-type number number
 */

int
ParseStation(struct lex_record *p)
{

	long      i, a1, b1, c1;
	char     *ptr;

	if (NextToken(p) != TOKEN_NUMBER) {
		Resync(p, Nil);
		return -1;
	}

	i = (long) (lex_value.double_value + 0.5);

	if (NextToken(p) != TOKEN_STRING) {
		Resync(p, Nil);
		return -1;
	}

	ptr = strdup(token);

	if (NextToken(p) != TOKEN_NUMBER) {
		Resync(p, Nil);
		return -1;
	}

	a1 = (long) (lex_value.double_value + 0.5);

	if (NextToken(p) != TOKEN_NUMBER) {
		Resync(p, Nil);
		return -1;
	}

	b1 = (long) (lex_value.double_value + 0.5);

	if (NextToken(p) != TOKEN_NUMBER) {
		Resync(p, Nil);
		return -1;
	}

	c1 = (long) (lex_value.double_value + 0.5);

	push_value(p, long_value, c1);
	push_value(p, long_value, b1);
	push_value(p, long_value, a1);
	push_value(p, string_value, ptr);
	push_value(p, long_value, i);

	return 0;
}

/*
 *  Parse syntax:  n.n.n.n.n.n.n
 */

int
ParseDISEntityType(struct lex_record *p)
{

	long      i, av;
	field_id  t;

	for (i = 0; i < 7; ++i) {

	        t = NextToken(p);

		if (t != TOKEN_NUMBER) {
		  if (t == EndOfFile) {
		    return -2;
		  }
		  Resync(p, Nil);
		  return -1;
		}

		if (lex_value.double_value > 0) {
		  av = (long) (lex_value.double_value + 0.5);
		}
		else {
		   av = (long) (lex_value.double_value - 0.5);
		}
		push_value(p, long_value, av);

		if (i < 6 && NextToken(p) != TOKEN_COLON) {
			Resync(p, Nil);
			return -1;
		}
	}

	return 0;
}
