#pragma once

#include <map>

#include "uimap/SuperCellMapView.h"
#include "chat/chatHandle.h"

struct vehVehicle;

/************************************************************************************************
	PlayerLayer
*************************************************************************************************/

// handle, when to blink till (in frametime)
using PlayerBlinkMap = std::map<ChatHandle, SINT32>;

class PlayerLayer;

class VehicleMapObject : public SuperCellMapView::MapObject
{
public:
	VehicleMapObject(PlayerLayer*, unsigned long long handle);
	virtual void DrawSelf(SuperCellMapView*);
	virtual void Over(SuperCellMapView*,int state);
	virtual bool IsHit(SuperCellMapView*, float x, float y) const;

	void HandleEmoteEvent(SuperCellMapView*, int emote);

	unsigned long long _handle; // either a host index or chat handle depending on _useMissionMemberUpdate
	unsigned int _nextsquadcheck;
	int _lastupdatetime;
	double _x, _y, _z;
	float _maxDraw; // max draw distance
	int _type, _class;
	wchar_t _callsign[MAX_HANDLE_WSIZE];
	wchar_t _id[64];
	bool _dead;
	bool _showinfo;
	bool _inmissionupdate;
	bool _issquadmate;
	bool _issamemission;
	PlayerLayer* _players;
};

class PlayerLayer : public SuperCellMapView::Renderer
{
public:
    PlayerLayer();
	~PlayerLayer() {};

	virtual void RenderSuperCells(SuperCellMapView*, int scx1, int scy1, int scx2, int scy2);
	virtual SuperCell* NewSuperCell(Renderer* r, int scx, int scy) { return 0; }

	using VehicleMapID = std::pair<UINT64,bool>;
	using VehicleMap = std::map<VehicleMapID, phui::ref_ptr<VehicleMapObject> >;

	VehicleMap _vehicles;
	int _timeout;
	//bool _useMissionMemberUpdate;
	//bool _showOnlyMissionMemberOnVisList;
	int _icontexid;

	static PlayerBlinkMap blinkMap;
	static SINT32 GetBlinkTime(ChatHandle handle);
};
