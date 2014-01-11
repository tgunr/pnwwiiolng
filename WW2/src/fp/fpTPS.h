#pragma once

// distances out to which we can see a given LOD .. i.e. we see LOD (n) between dist(n-1) and dist(n)
#define TPS_LOD1_DISTANCE	10.0
#define TPS_LOD2_DISTANCE	50.0
#define	TPS_LOD3_DISTANCE	100.0

#define TPS_LOD1_DISTANCE2	100.0
#define TPS_LOD2_DISTANCE2	2500.0
#define	TPS_LOD3_DISTANCE2	10000.0


#define _FPS_COMPONENT_HEAD 0
#define _FPS_COMPONENT_TORSO 1
#define _FPS_COMPONENT_ARMS 2
#define _FPS_COMPONENT_LEGS 3

// typedefs & prototypes
struct gr2AnimationBlend;

enum fpHitLocations
{
	FP_HIT_LOCATION_NONE = -1,
	FP_HIT_LOCATION_HEAD,
	FP_HIT_LOCATION_TORSO,
	FP_HIT_LOCATION_ARMS,
	FP_HIT_LOCATION_LEGS,
	FP_HIT_LOCATION_NUM
};

typedef std::map<UINT64, gr2AnimationBlend*> TPS_BLEND_MAP;

extern fpHitLocations	fpHitMap[60];

extern LOGICAL	fpTPSLoad (void);
extern void	fpTPSKill (void);

// using tps weapon types
extern const char* fpTPSCreateBlendKey(int pose, int move, int weap, int point, int type, bool* exists = nullptr);
extern UINT64 fpTPSCreateBlendID(int pose, int move, int weap, int point, int type = 0);
extern gr2AnimationBlend* fpTPSGetBlend(int pose, int move, int weap, int point, int type = 0);

extern void fpTPSFireRound(void* FireVehicle);
#ifdef GR2_BLEND_EDITOR
extern TPS_BLEND_MAP& fpTPSGetBlends();
#endif
