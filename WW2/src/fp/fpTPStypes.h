#pragma once

#define VEH_MAX_MOUNTS 16

// these are the offsets so we can get the appropriate UI string from the loc file
#define			FPS_BODYEMOTE_OFFSET	10000
#define			FPS_COMBATEMOTE_OFFSET	10100
#define			TPS_BODYEMOTE_OFFSET	11000
#define			TPS_COMBATEMOTE_OFFSET	11100

#define			TPS_HEAD_STRAIGHT	0
#define			TPS_HEAD_LEFT		1
#define			TPS_HEAD_RIGHT		2

#define			TPS_NOT_DEAD		0
#define			TPS_DEATH1			1
#define			TPS_DEATH2			2
#define	        TPS_DEATH3          3
#define         TPS_DEATH4          4
#define			TPS_NUM_DEATHS		5


#define _4_BYTE(val)	(((val) >> 24) && 1)
#define _3_BYTE(val)	(((val) >>16) && 1)
#define _2_BYTE(val)	(((val) >> 8) && 1)
#define _1_BYTE(val)	((val) && 1)
#define _NUM_BYTE(val) _1_BYTE(val) + _2_BYTE(val) + _3_BYTE(val) + _4_BYTE(val)

#define _MOD_BYTE(val) 	((val) & 0xFF)

#define _2_NIBBLE(val) (((val) >> 4) && 1)
#define _1_NIBBLE(val) ((val) && 1)
#define _NUM_NIBBLE(val)  _2_NIBBLE(_MOD_BYTE(val)) + _1_NIBBLE(_MOD_BYTE(val))

#define _MOD_NIBBLE(val) ((val) & 0x0F)

#define _8_BIT(val) (((val) >> 7) && 1)
#define _7_BIT(val) (((val) >> 6) && 1)
#define _6_BIT(val) (((val) >> 5) && 1)
#define _5_BIT(val) (((val) >> 4) && 1)
#define _4_BIT(val) (((val) >> 3) && 1)
#define _3_BIT(val) (((val) >> 2) && 1)
#define _2_BIT(val) (((val) >> 1) && 1)
#define _1_BIT(val) ((val) && 1)

#define _NUM_BIT(val) _8_BIT(val)\
					+ _7_BIT(val)\
					+ _6_BIT(val)\
					+ _5_BIT(val)\
					+ _4_BIT(val)\
					+ _3_BIT(val)\
					+ _2_BIT(val)\
					+ _1_BIT(val)

#define NUM_BITS(val)(_NUM_BIT((val) >> 24) + _NUM_BIT((val) >> 16) + _NUM_BIT((val) >> 8)  + (_NUM_BIT(val)))

#define TPS_EMOTE_BITS	NUM_BITS(TPS_BODYEMOTE_NUM_TYPES +TPS_COMBATEMOTE_NUM_TYPES)
#define TPS_DEATH_BITS	2
#define TPS_LEFT_BITS		1
#define TPS_RIGHT_BITS		1
#define TPS_POINT_BITS		NUM_BITS(TPS_POINTTYPE_NUM_TYPES-2) // one extra on the end that shouldn't be used
#define TPS_MOVE_BITS		NUM_BITS(TPS_MOVETYPE_NUM_TYPES-3)  // 2 turn movetypes + num types not used
#define TPS_POSE_BITS		NUM_BITS(TPS_POSETYPE_NUM_TYPES-1)
#define TPS_WEAP_BITS		NUM_BITS(VEH_MAX_MOUNTS-1)
#define TPS_ALT_BITS		1

#define TPS_ALT_BIT_SHIFT	0
#define TPS_EMOTE_BIT_SHIFT	(TPS_ALT_BIT_SHIFT + TPS_ALT_BITS)
#define TPS_DEATH_BIT_SHIFT	(TPS_EMOTE_BIT_SHIFT + TPS_EMOTE_BITS)
#define TPS_LEFT_BIT_SHIFT	(TPS_DEATH_BIT_SHIFT + TPS_DEATH_BITS)
#define TPS_RIGHT_BIT_SHIFT	(TPS_LEFT_BIT_SHIFT  + TPS_LEFT_BITS)
#define TPS_POINT_BIT_SHIFT	(TPS_RIGHT_BIT_SHIFT + TPS_RIGHT_BITS)
#define TPS_MOVE_BIT_SHIFT	(TPS_POINT_BIT_SHIFT + TPS_POINT_BITS)
#define TPS_POSE_BIT_SHIFT	(TPS_MOVE_BIT_SHIFT  + TPS_MOVE_BITS)
#define TPS_WEAP_BIT_SHIFT	(TPS_POSE_BIT_SHIFT  + TPS_POSE_BITS)
#define TPS_TOTAL_BITS		(TPS_ALT_BIT_SHIFT 	 + TPS_ALT_BITS)

#define _CREATE_MASK(bits)	(0xFFFFFFFF >> (32 - bits))
#define CREAT_MASK_SHIFT(shift,bits) (_CREATE_MASK(bits) << shift)

#define TPS_DEATH_MASK	CREAT_MASK_SHIFT(TPS_DEATH_BIT_SHIFT,TPS_DEATH_BITS)
#define TPS_LEFT_MASK	CREAT_MASK_SHIFT(TPS_LEFT_BIT_SHIFT, TPS_LEFT_BITS)
#define TPS_RIGHT_MASK	CREAT_MASK_SHIFT(TPS_RIGHT_BIT_SHIFT,TPS_RIGHT_BITS)
#define TPS_POINT_MASK	CREAT_MASK_SHIFT(TPS_POINT_BIT_SHIFT,TPS_POINT_BITS)
#define TPS_MOVE_MASK	CREAT_MASK_SHIFT(TPS_MOVE_BIT_SHIFT, TPS_MOVE_BITS)
#define TPS_POSE_MASK	CREAT_MASK_SHIFT(TPS_POSE_BIT_SHIFT, TPS_POSE_BITS)
#define TPS_WEAP_MASK	CREAT_MASK_SHIFT(TPS_WEAP_BIT_SHIFT, TPS_WEAP_BITS)
#define TPS_EMOTE_MASK	CREAT_MASK_SHIFT(TPS_EMOTE_BIT_SHIFT, TPS_EMOTE_BITS)
#define TPS_ALT_MASK	CREAT_MASK_SHIFT(TPS_ALT_BIT_SHIFT, TPS_ALT_BITS)
