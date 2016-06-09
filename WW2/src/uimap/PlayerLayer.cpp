#include <syscfg.h> // Path is project specific.
#include <string>

#include "phui/GraphicsContext.h"
#include "phui/MessageLoop.h"
#include "phui/UIMessages.h"
#include "phui/Message.h"

#include "uimap/PlayerLayer.h"
#include "veh/vehmain.h"
#include "ui/uiMain.h"
#include "sdm/sdm.h"
#include "pers/persmain.h"
#include "client/GameClient.h"
#include "chat/chatMissionMember.h"
#include "chat/chatHandle.h"
#include "veh/vehtype.h"
#include "include/vars.h"
#include "trn/trnmain.h"
#include "math/math3d.h"
#include "client/ClientSquad.h"
#include "fp/fpTPSdata.h" // emote types
#include "miss/missionDetail.h"
#include "scen/ScenarioManager.h"

/************************************************************************************************
	PlayerLayer
*************************************************************************************************/

PlayerLayer::PlayerLayer() : _timeout(10000), _icontexid(-1) //, _useMissionMemberUpdate(true), _showOnlyMissionMemberOnVisList(false),
{
	minscale = 0;
}

PlayerBlinkMap PlayerLayer::blinkMap;

SINT32 PlayerLayer::GetBlinkTime(ChatHandle handle)
{
	PlayerBlinkMap::iterator found = PlayerLayer::blinkMap.find(handle);
	if(found != PlayerLayer::blinkMap.end() && found->second > frametime)
	{
		return (SINT32)((found->second - frametime)/1000);
	}

	return 0;
}

/************************************************************************************************
	RenderSuperCells
*************************************************************************************************/

void PlayerLayer::RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2)
{
	_mv = mv;

	if(SCEN_MGR->IsActive())
	{
		SCENPLAYER_MAP& players = SCEN_MGR->GetPlayers();
		SCENPLAYER_MAP::iterator it = players.begin();
		ScenarioPlayer* me = SCEN_MGR->GetPlayer();

		for(; it != players.end(); ++it)
		{
			ScenarioPlayer& player = it->second;

			if(false && player.handle == me->handle)
				continue;
			else if(!player.GetSpawned())
				continue;
			else if(!player.IsActive())
				continue;

			bool teamMate = (player.GetTeam() == me->GetTeam());

			//if(!teamMate && !SCEN_MGR->HasFlag(ScenarioManager::ENEMY_ICONS))
			//	continue
			if(!teamMate)
				continue;

			const vehType* vtype = vehTypeGetBySpec(player.GetWeaponSpec());

			if(vtype && vtype->Type == VEH_TYPE_FLOATER)
				continue;

			// create a unique id, true means a mission member
			VehicleMapID vid = std::make_pair(player.handle, teamMate);

			// see if we're already in the map
			VehicleMapObject* vehobj = _vehicles[vid].ptr();

			if(!vehobj)
			{
				vehobj = new VehicleMapObject(this, player.handle);
				_vehicles[vid] = vehobj;

				MapObjects()->push_back(vehobj);

				vehobj->_visible = true;
				vehobj->_issamemission = teamMate;
				vehobj->SetBlink(_mv, PlayerLayer::GetBlinkTime(player.handle));
			}

			if(vtype)
			{
				vehobj->_type = vtype->Type;
				vehobj->_class = vtype->WeaponSpec.wClass;
				wcsncpy(vehobj->_id, vehTypeLoc(3, vtype), 63);
			}

			vehobj->_x = player.GetX();
			vehobj->_y = player.GetY();

			//wcsncpy(vehobj->_callsign, chatGetWideCallsign(member.m_handle), MAX_HANDLE_WLEN);
			wcsncpy(vehobj->_callsign, chatGetWideCallsign(player.handle), MAX_HANDLE_WSIZE); // fixed -AGAIN- aw
			vehobj->_lastupdatetime = frametime; //mv->ParentLoop()->FrameTime();
			vehobj->_inmissionupdate = true;
		}
	}
	else
	{
		vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

		std::vector<MissionMember>::const_iterator it = PlayerClient()->MissionMembers().begin();
		for(; it != PlayerClient()->MissionMembers().end(); ++it)
		{
			MissionMember const& member = *it;
			if(PlayerVehicle && PlayerVehicle->Chat_Handle == member.m_handle)
				continue;

			if(!member.isSpawned())
				continue;

			vehType const* vehtype = vehTypeGetBySpec(*(WEAPON_SPEC*)&member.m_weapon);

			if(vehtype && vehtype->Type == VEH_TYPE_FLOATER)
				continue;

			// create a unique id, true means a mission member
			VehicleMapID vid = std::make_pair(member.m_handle,true);

			// see if we're already in the map
			VehicleMapObject* vehobj = _vehicles[vid].ptr();

			if(!vehobj)
			{
				vehobj = new VehicleMapObject(this, member.m_handle);
				_vehicles[vid] = vehobj;

				MapObjects()->push_back(vehobj);
				vehobj->_visible = true;
				vehobj->_issamemission = true;

				// air guys on the same mission can see other air guys a long way
				if(PlayerVehicle && vehtype)
				{
					if((PlayerVehicle->VehicleType->Type == VEH_TYPE_AIR && vehtype->Type == VEH_TYPE_AIR) ||
						(PlayerVehicle->VehicleType->Type == VEH_TYPE_SEA && vehtype->Type == VEH_TYPE_SEA))
						vehobj->_maxDraw = 1000000.0f;
				}

				vehobj->SetBlink(_mv, PlayerLayer::GetBlinkTime(member.m_handle));
			}

			if(vehtype)
			{
				vehobj->_type = vehtype->Type;
				vehobj->_class = vehtype->WeaponSpec.wClass;
				wcsncpy(vehobj->_id, vehTypeLoc(3, vehtype), 63);
			}

			vehobj->_x = member.m_x;
			vehobj->_y = member.m_y;

			//wcsncpy(vehobj->_callsign, chatGetWideCallsign(member.m_handle), MAX_HANDLE_WLEN);
			wcsncpy(vehobj->_callsign, chatGetWideCallsign(member.m_handle), MAX_HANDLE_WSIZE); // fixed -AGAIN- aw
			vehobj->_lastupdatetime = frametime; //mv->ParentLoop()->FrameTime();
			vehobj->_inmissionupdate = true;
		}

		// lets show all other friendlies
		if(PlayerVehicle)
		{
			vehVehicle* Vehicle = nullptr;
			vehGetVehicleHead(&Vehicle);

			for(; Vehicle; Vehicle = Vehicle->getNext())
			{
				if(!Vehicle->InUse) continue;
				if(Vehicle->isHulked()) continue;
				if(Vehicle == PlayerVehicle) continue;
				if(!vehIconCanSeeHandle(Vehicle)) continue;
				//if(_showOnlyMissionMemberOnVisList && !Vehicle->SameMission) continue;
				if(Vehicle->SameMission) continue; // mission members are handled in block above
				if(Vehicle->VehicleType->Type == VEH_TYPE_FLOATER) continue; // don't show floaters

				// create a unique id, false means not a mission member
				VehicleMapID vid = std::make_pair<UINT64,bool>((UINT64)Vehicle->VehicleHostIndex,false);

				VehicleMapObject* vehobj = _vehicles[vid].ptr();
				if(!vehobj)
				{
					vehobj = new VehicleMapObject(this, Vehicle->VehicleHostIndex);
					_vehicles[vid] = vehobj;

					MapObjects()->push_back(vehobj);
					vehobj->_visible = true;
					vehobj->_type = Vehicle->VehicleType->Type;
					vehobj->_class = Vehicle->VehicleType->WeaponSpec.wClass;
					//wcsncpy(vehobj->_callsign, chatGetWideCallsign(Vehicle->ChatHandle), MAX_HANDLE_WLEN);
					wcsncpy(vehobj->_callsign, chatGetWideCallsign(Vehicle->Chat_Handle), MAX_HANDLE_WSIZE); // fix for 10 char bug I think - AW

					if(Vehicle->isSTO() && Vehicle->VehicleType->WeaponSpec.wClass == 14) // Bailed air crew
						wcsncpy(vehobj->_id, uiLocString("vehicles.bailedcrew").c_str(), 63);
					else
						wcsncpy(vehobj->_id, vehTypeLoc(3, Vehicle->VehicleType), 63);

					vehobj->_lastupdatetime = frametime; //mv->ParentLoop()->FrameTime();
					vehobj->SetBlink(_mv, PlayerLayer::GetBlinkTime(Vehicle->Chat_Handle));
				}
				else
				{
					vehobj->_lastupdatetime = frametime; //mv->ParentLoop()->FrameTime();
				}
			}
		}
	}

	MapObjects()->Draw(mv);
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

 VehicleMapObject::VehicleMapObject(PlayerLayer* layer, unsigned long long handle)
			: SuperCellMapView::MapObject()
			, _handle(handle) , _nextsquadcheck(0), _lastupdatetime(0)
			, _x(0), _y(0), _z(0)
			, _maxDraw(100000.0f), _type(0), _class(0), _dead(false), _showinfo(false)
			,  _inmissionupdate(false), _issamemission(false), _players(layer)

{
	*_callsign = 0;
	*_id = 0;
	_issquadmate = PlayerClient()->PlayerSquad().isMember(_handle);
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/
#define ICONSIZE 8.0f
#define ICONSIZE2 4.0f
#define SQR(x) ((x)*(x))
#define SQUAD_CHECK_INTERVAL 10000 // every 10 seconds

void VehicleMapObject::DrawSelf(SuperCellMapView* mv)
{
    FLOAT64VECTOR vecWorldOriginOffset;

	FLOAT64 playerx = 0;
	FLOAT64 playery = 0;
	//unsigned int frameTime = mv->ParentLoop()->FrameTime();

	// lets get my position
   	trnGetWorldOriginOffset(&vecWorldOriginOffset);

    playerx = -playpos.x + vecWorldOriginOffset.x;
    playery = -playpos.y + vecWorldOriginOffset.y;

	// lets get my friend's position
	vehVehicle*	veh = nullptr;
	if(!_issamemission)
		veh = vehFindVehicle((UINT32)_handle); // vehicle host index
	else
		veh = vehFindVehicleByChatHandle(_handle); // chat handle

	if(veh)
	{
		_x = veh->PosData.RenderPos.x;
		_y = veh->PosData.RenderPos.y;
		_z = veh->PosData.RenderPos.z;
	}

	// are they too far away to show?
	if(math3dFastSqrt(SQR((float)_x - (float)playerx) + SQR((float)_y - (float)playery)) > _maxDraw)
		return;

	phui::GraphicsContext* ctx = mv->GetGraphicsContext();
	ctx->PushState();

	float r,g,b;
	phui::BoxVerts box;
	float iconscale = mv->Width() / mv->Scale();
	if(mv->Scale() > 8.0f)
		iconscale = mv->Width()/8.0f;
	r = g = b = 0;

	iconscale -= iconscale * .45f;

	phui::box_verts_with_pos(box, (float)_x/SDM_SUPERCELL_SIZE-ICONSIZE2/iconscale,  (float)_y/SDM_SUPERCELL_SIZE-ICONSIZE2/iconscale
								, ICONSIZE/iconscale, ICONSIZE/iconscale);

	if(_handle == PlayerClient()->SelectedMission().m_poster)
	{
		r = piRGBA_GETRED(gIconMissionLeaderColor)/(float)0xff;
		g = piRGBA_GETGREEN(gIconMissionLeaderColor)/(float)0xff;
		b = piRGBA_GETBLUE(gIconMissionLeaderColor)/(float)0xff;
	}
	else if(_issquadmate)
	{
		r = piRGBA_GETRED(gIconSquadColor)/(float)0xff;
		g = piRGBA_GETGREEN(gIconSquadColor)/(float)0xff;
		b = piRGBA_GETBLUE(gIconSquadColor)/(float)0xff;
	}
	else if(_issamemission)
	{
		r = piRGBA_GETRED(gIconMissionColor)/(float)0xff;
		g = piRGBA_GETGREEN(gIconMissionColor)/(float)0xff;
		b = piRGBA_GETBLUE(gIconMissionColor)/(float)0xff;
	}
	else
	{
		if(veh == nullptr)
		{
			if(r) r *= .9f;
			else r = .4f;

			if(g) g *= .9f;
			else g = .4f;

			if(b) b *= .9f;
			else b = .4f;

			// force a timeout
			_lastupdatetime = 0;
		}
		else
		{
			vehIconColor3f(veh, &r, &g, &b);
		}
	}

	ctx->SetColor(r,g,b);
	if(1)
	{
		const float* uv;
		// Make 'em const so that it doesn't put them onto the stack
		// every time you come thru this loop. 6 * 8 * 4 = 96 less bytes on the stack...
		static const float air[8]		= {0,.75, .25,.75, .25,1,  0,1};
		static const float sea[8]		= {.25,.75, .5,.75, .5,1,  .25,1};
		static const float ground[8]	= {0,.5, .25,.5, .25,.75,  0,.75};
		static const float inf[8]		= {.25,.5, .5,.5, .5,.75,  .25,.75};
		static const float gun[8]		= {.5,.75, .75,.75, .75,1,  .5,1};
		static const float truck[8]		= {.5,.5, .75,.5, .75,.75,  .5,.75};
//		float danger[8]	= {.75,.75, 1,.75, 1,1,  .75,1};

		if(_type == VEH_TYPE_GROUND && _class == 4) // only tanks
			uv = ground;
		else if(_type == VEH_TYPE_GROUND && _class == 7) // only guns
			uv = gun;
		else if(_type == VEH_TYPE_GROUND && _class == 6) // only trucks
			uv = truck;
		else if(_type == VEH_TYPE_SEA)
			uv = sea;
		else if(_type == VEH_TYPE_AIR)
			uv = air;
		else
			uv = inf;

		int texid = _players->_icontexid;

		ctx->EnableTexture();
		ctx->SetTexture(texid);
		phui::draw_gl_quad(box.verts, uv);
	}
	else
	{
		ctx->DisableTexture();
		phui::draw_gl_quad(box.verts);
	}

	if((unsigned int)frametime > _nextsquadcheck)
	{
		_issquadmate	= PlayerClient()->PlayerSquad().isMember(_handle);
		_nextsquadcheck = frametime + SQUAD_CHECK_INTERVAL;
	}


	if(abs((long)(frametime - _lastupdatetime)) > _players->_timeout  || (_issamemission && !_inmissionupdate) )
	{
		// create a unique id
		PlayerLayer::VehicleMapID vid = std::make_pair(_handle, _issamemission);

		// remove it from the layer
		_players->MapObjects()->remove(this);

		// remove it from the vehicle map
		PlayerLayer::VehicleMap::iterator it = _players->_vehicles.find(vid);
		if(it != _players->_vehicles.end()) _players->_vehicles.erase(it);
	}

	ctx->SetColor(1,1,1,1);
	ctx->PopState();
	_inmissionupdate = false;
}

/************************************************************************************************
	Over
*************************************************************************************************/

void VehicleMapObject::Over(SuperCellMapView* mv, int state)
{
	bool wasover = _showinfo;

	_showinfo = (state != phui::MOUSE_EXIT);

	if(wasover != _showinfo)
	{
		if(_showinfo)
		{
			std::wstring tip;

			tip += _callsign;

			if(*_id)
			{
				tip += L" <";
				tip += _id;
				tip += L">";
			}

			mv->SetToolTip(tip.c_str());
		}
		else
		{
			mv->SetToolTip(L"");
			mv->ParentViewport()->HideToolTip();
		}
	}
}

/************************************************************************************************
	IsHit
*************************************************************************************************/

bool VehicleMapObject::IsHit(SuperCellMapView* mv, float x, float y) const
{
	float iconscale = mv->Width() / mv->Scale();
	float bottom = (float)_y / SDM_SUPERCELL_SIZE - 8.0f/iconscale;
	float left = (float)_x / SDM_SUPERCELL_SIZE - 8.0f/iconscale;
	float top = bottom + 16.0f/iconscale;
	float right = left + 16.0f/iconscale;

	return (x >= left && x <= right && y >= bottom && y <= top);
}

void VehicleMapObject::HandleEmoteEvent(SuperCellMapView* mv, int emote)
{
	switch(emote)
	{
		case TPS_COMBATEMOTE_AMMO:
			SetBlink(mv, 30); // blink for 30 seconds
		break;
		default:
			// do nothing
		break;
	}
}
