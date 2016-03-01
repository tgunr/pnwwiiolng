#pragma once

enum
{
	VEHICON_RECRUIT,
	VEHICON_PRIVATE,
	VEHICON_PFC,
	VEHICON_CORPORAL,
	VEHICON_SGT,
	VEHICON_SGT1,
	VEHICON_SGT2,
	VEHICON_SGT3,

	VEHICON_2NDLT,
	VEHICON_1STLT,
	VEHICON_CAPTAIN,
	VEHICON_MAJOR,
	VEHICON_COL,
	VEHICON_LTCOL,
	VEHICON_GEN1,
	VEHICON_GEN2,

	VEHICON_GEN3,
	VEHICON_GEN4,
	VEHICON_GEN5,
	VEHICON_COUNTRY_LEADER,
	VEHICON_THEATER_LEADER,
	VEHICON_SIDE_COMMANDER,
	VEHICON_RAT1,
	VEHICON_RAT2,

	VEHICON_RAT3,
	VEHICON_EXCLAMATION,
	VEHICON_THUNDER,
	VEHICON_KILLER,
	VEHICON_KICKME,
	VEHICON_APPLE,
	VEHICON_MARTINI,
	VEHICON_TOTO,

	VEHICON_FT,
	VEHICON_SNAIL,
	VEHICON_LINDIR,
	VEHICON_HC,
	VEHICON_BUILDER,
	VEHICON_TRAINER,
	VEHICON_MISSIONLEADER,
	VEHICON_PULSE,

	VEHICON_XOOM,
	VEHICON_VICTARUS,
};

struct vehVehicle;

extern SINT32 	vehIconInit(void);
extern SINT32 	vehIconKill(void);
extern SINT32 	vehIconUpdate(struct vehVehicle*);
extern SINT32 	vehIconDrawDeprecated(void);
extern SINT32	vehIconMode(void);
extern SINT32 	vehIconDraw2DRank(UINT32 Rank, SINT32 x, SINT32 y, LOGICAL colorIcon);
extern LOGICAL	vehIconCanSeeHandle(struct vehVehicle*);
extern UINT32	vehIconColor(struct vehVehicle*);
extern void	vehIconColor3f(struct vehVehicle*, float* r, float* g, float* b);
extern void	vehSetIconMode(SINT32 mode);
extern void	vehIconSetPulsate(struct vehVehicle*, SINT32 duration); // pulses the icon for a period of time; duration is in ms
extern void vehIconDraw();
