#include <syscfg.h> // Path is project specific.
#include "phui/GraphicsContext.h"
#include "phui/Util.h"
#include "phui/UIMessages.h"
#include "phui/MessageLoop.h"
#include "phui/Viewport.h"

#include "include/vars.h"
#include "gl/glplat.h"
#include "uimap/WaypointLayer.h"
#include "chat/chatWaypointExchg.h"
#include "sdm/sdm.h"
#include "client/GameClient.h"
#include "chat/chatHandle.h"
#include "ui/uiMain.h"
#include "teul/teulTime.h"
#include "sdm/StratDB.h"
#include "pers/persmain.h"

// in seconds
#define CONTACT_REPORT_FADEOUT 600.0f
#define UNAPPROVED_REPORT_TIMEOUT 30
#define DEATH_FADEOUT 120.0f

/************************************************************************************************
	WaypointLayer
*************************************************************************************************/

WaypointLayer::WaypointLayer():
	_waypointtexid(-1),
	_mortartexid(-1),
	_contacttexid(-1),
	_markertexid(-1),
	_aiCount(0),
    _aiInit(true)
{
	minscale = 0;
}

FULL_OBJ_ID WaypointLayer::nearestCP;

/************************************************************************************************
	RenderSuperCells
*************************************************************************************************/

void WaypointLayer::RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2)
{
	_mv = mv;

	// lookup the player vehicle
	vehVehicle* player = playerGetVehicle();

	// redo all of the ai waypoints?
	bool setAI = (player && (_aiInit || memcmp(&nearestCP, &player->ModelData->closestCp, sizeof(FULL_OBJ_ID))));

	// remove ai waypoints
	if(setAI)
	{
		// get a copy so removal doesn't corrupt source vector
		std::vector<Waypoint> aiwps = PlayerClient()->Waypoints();

		// remove all ai waypoints
		for(int i = 0; i < (int)aiwps.size(); i++)
		{
			Waypoint& wp = aiwps.at(i);

			if(wp.type() == WPT_FAI || wp.type() == WPT_EAI)
				PlayerClient()->RemoveWaypoint(wp.handle());
		}
	}

	const std::vector<Waypoint>& waypoints = PlayerClient()->Waypoints();
	std::vector<Waypoint>::const_iterator it = waypoints.begin();

	for(; it !=  waypoints.end(); ++it)
	{
		WaypointMap::iterator wayit = _waypoints.find(it->handle());
		WaypointMapObject* wayobj = 0;
		if(wayit == _waypoints.end())
		{
			if(it->type() != WPT_AMMO)
			{
				wayobj = new WaypointMapObject(this, *it);
				MapObjects()->push_back(wayobj);
				_waypoints[it->handle()] = wayobj;
			}
			else
			{
				// lets update the player layer with the bullet blink
				// if we're a truck or rifleman and we're close enough, send us a gameevent

				PlayerClient()->RemoveWaypoint(it->handle()); // its just a flag, so immediately remove it
			}
		}
		else
		{
			wayobj = wayit->second.ptr();
			wayobj->_waypoint = *it;
		}

		if(wayobj)
			wayobj->_remove = false;
	}

	// add ai waypoints
	if(false && setAI)
	{
		_aiInit = false;

		// save the new nearest cp
		memcpy(&nearestCP, &player->ModelData->closestCp, sizeof(FULL_OBJ_ID));

		// look up nearest cp
		StratDB::StratObject* cp = StratDB::FindStratObject(nearestCP);

		if(cp)
		{
			// get the ai list
			StratDB::StratObjList aiList;

			// look up the new set of ai contacts
			if(StratDB::FindStratObjects(aiList, SDM_CLASS_BUILDING, SDM_BUILDING_AUTOGUNNER, -1, cp))
			{
				char buffer[10+1];
				int aiCount = 0;

				StratDB::StratObjList::iterator ait = aiList.begin();
				for(; ait != aiList.end(); ++ait)
				{
					StratDB::StratObject* ai = *ait;
					UINT32 subtype = 0;

					switch(ai->SubType())
					{
						case SDM_AUTOGUNNER_MGNEST:
						case SDM_AUTOGUNNER_RIFLEMAN:
						case SDM_AUTOGUNNER_RIFLEPIT:
						case SDM_AUTOGUNNER_MORTARPIT:
						case SDM_AUTOGUNNER_ARTILLERYLIGHT:
						case SDM_AUTOGUNNER_ARTILLERYMEDIUM:
						case SDM_AUTOGUNNER_ARTILLERYHEAVY:
						case SDM_AUTOGUNNER_COASTDEFENSE:
						case SDM_AUTOGUNNER_AAALIGHT:
						case SDM_AUTOGUNNER_AAAMEDIUM:
						case SDM_AUTOGUNNER_AAAHEAVY:
						case SDM_AUTOGUNNER_ATLIGHT:
						case SDM_AUTOGUNNER_ATMEDIUM:
						case SDM_AUTOGUNNER_ATHEAVY:
						{
							subtype = WTP_ENEMY_GUN;
						}
						break;
						default:
							subtype = WTP_ENEMY_DANGER;
						break;
					}

					snprintf(buffer, 10, "AI%d", ++aiCount);

					StratDB::StratObject* parent = StratDB::FindStratObject(ai->ParentId());

					WP_TYPE type = (parent && parent->Side() == player->Side) ? WPT_FAI: WPT_EAI;

					Waypoint wp(true, chatGetHandle(buffer), 0, type, subtype, ai->PosX(), ai->PosY(), 0, "ai gun");
					wp.setShared(false);

					// add this waypoint
					PlayerClient()->ProcessWaypoint(wp, WPXT_POST);
				}
			}
		}
	}

	MapObjects()->Draw(mv);
}

/************************************************************************************************
	WaypointMapObject
*************************************************************************************************/

WaypointMapObject::WaypointMapObject(WaypointLayer* l, Waypoint const& w)
		: _waypoint(w), _remove(true), _waypointlayer(l), _mouseover(false), _on(true), _lasttime(0)
{
	_visible = true;

	const wchar_t* wname = chatGetWideCallsign(chatGetLowercaseHandle(w.handle()));

	std::string key = "waypoints.";
	key += UTF8(wname);

	_name = uiLocString(key.c_str());

	if(_name[0] == L'?')
		_name = chatGetWideCallsign(w.handle());
}

#define ICONSIZE 16.0f

void WaypointMapObject::DrawSelf(SuperCellMapView* mv)
{
	UINT32 lifetime = teulTime() - _waypoint.posted();

	if(_waypoint.local())
	{
		if(_waypoint.type() == WPT_ENEMY || _waypoint.type() == WPT_MARKER)
		{
			unsigned int now = (unsigned int)frametime; //mv->ParentLoop()->FrameTime();

			if((now - _lasttime) > 500)
			{
				_on = !_on;
				_lasttime = now;
			}
		}

		if(_waypoint.shared())
		{
			if(lifetime >= UNAPPROVED_REPORT_TIMEOUT)
			{
				PlayerClient()->RemoveWaypoint(_waypoint.handle());
				_remove = true;
			}
		}

		if(_waypoint.type() == WPT_DEATH && (float)lifetime >= DEATH_FADEOUT)
		{
			PlayerClient()->RemoveWaypoint(_waypoint.handle());
			_remove = true;
		}
	}
	else
	{
		_on = true;

		if(_waypoint.type() == WPT_ENEMY && (float)lifetime >= CONTACT_REPORT_FADEOUT)
		{
			PlayerClient()->RemoveWaypoint(_waypoint.handle());
			_remove = true;
		}
	}

	if(_remove)
	{
		_waypointlayer->MapObjects()->remove(this);

		WaypointLayer::WaypointMap::iterator it = _waypointlayer->_waypoints.find(_waypoint.handle());
		if(it != _waypointlayer->_waypoints.end())
			_waypointlayer->_waypoints.erase(it);

		return;
	}

	_remove = true;

	if(!uiInGame() && !PlayerClient()->IsMissionLeader() && _waypoint.type() != WPT_ORIGIN && _waypoint.type() != WPT_TARGET)
		return;

	if(!_on)
		return;

	float iconscale = mv->Width() / mv->Scale();
	float iconsize = ICONSIZE / iconscale;
	float iconx = _waypoint.x()/SDM_SUPERCELL_SIZE - iconsize/2.0f;
	float icony = _waypoint.y()/SDM_SUPERCELL_SIZE - iconsize/2.0f;

	phui::GraphicsContext* ctx = mv->GetGraphicsContext();

	float* uv = 0;
	float full[8]		= {0,0,1,0,1,1,0,1};
	float air[8]		= {0,.75, .25,.75, .25,1,  0,1};
	float sea[8]		= {.25,.75, .5,.75, .5,1,  .25,1};
	float ground[8]		= {0,.5, .25,.5, .25,.75,  0,.75};
	float inf[8]		= {.25,.5, .5,.5, .5,.75,  .25,.75};
	float gun[8]		= {.5,.75, .75,.75, .75,1,  .5,1};
	float truck[8]		= {.5,.5, .75,.5, .75,.75,  .5,.75};
	float danger[8]		= {.75,.75, 1,.75, 1,1,  .75,1};
	float unknown[8]	= {.75,.5, 1,.5, 1,.75,  .75,.75};
	float mortar[8]		= {0,.25, .25,.25, .25,.5,  0,.5};

	phui::BoxVerts box;
	phui::box_verts_with_pos(box, iconx, icony, iconsize, iconsize);

	int texid = -1;
	bool drawName = true;



	switch(_waypoint.type())
	{
		case WPT_MOBILESPAWN:
		case WPT_ORIGIN:
		case WPT_RALLY:
			ctx->SetColor(0,0,1);
			texid = _waypointlayer->_waypointtexid;
			uv = full;
		break;
		case WPT_MARKER:
			ctx->SetColor(0,0,1.0f,.9f);
			texid = _waypointlayer->_markertexid;
			uv = full;
		break;
		case WPT_TARGET:
		case WPT_ATTACK:
			ctx->SetColor(1,0,0);
			texid = _waypointlayer->_waypointtexid;
			uv = full;
		break;
		case WPT_BOMB:
			ctx->SetColor(1,1,1);
			texid = _waypointlayer->_mortartexid;
			uv = full;
		break;
		case WPT_ENEMY:
		case WPT_EAI:
		case WPT_FAI:
		{
			float alpha = 1.0f - ((float)lifetime / CONTACT_REPORT_FADEOUT);

			if(_waypoint.type() == WPT_EAI || _waypoint.type() == WPT_FAI)
				alpha = 1.0f;

			if(alpha > 0) // alpha should always be greater than 0 if we get to here
			{
				UINT32 color = (_waypoint.type() == WPT_FAI) ?
									gIconFriendlyColor:
                                    colorLavender;
                
				float r = piRGBA_GETRED(color)/(float)0xff;
				float g = piRGBA_GETGREEN(color)/(float)0xff;
				float b = piRGBA_GETBLUE(color)/(float)0xff;

				ctx->SetColor(r, g, b, alpha);
				texid = _waypointlayer->_contacttexid;

				if(_waypoint.subType() == WTP_ENEMY_AIR)
					uv = air;
				else if(_waypoint.subType() == WTP_ENEMY_LAND)
					uv = ground;
				else if(_waypoint.subType() == WTP_ENEMY_SEA)
					uv = sea;
				else if(_waypoint.subType() == WTP_ENEMY_GUN)
					uv = gun;
				else if(_waypoint.subType() == WTP_ENEMY_TRUCK)
					uv = truck;
				else if(_waypoint.subType() == WTP_ENEMY_DANGER)
					uv = danger;
				else if(_waypoint.subType() == WTP_ENEMY_UNKNOWN)
					uv = unknown;
				else if (_waypoint.subType() == WTP_ENEMY_MORTAR)
					uv = mortar; // Mortar barrage contact report; DIFFERENT from mortar barrage waypoint!
				else
					uv = inf;
			}

			drawName = false;
		}
		break;
		case WPT_DEATH:
		{
			float alpha = 1.0f - ((float)lifetime / DEATH_FADEOUT);

			if(alpha > 0) // alpha should always be greater than 0 if we get to here
			{
				ctx->SetColor(1.0f, 1.0f, 1.0f, alpha);

				texid	= _waypointlayer->_contacttexid;
				uv		= danger;
			}

			drawName = false;
		}
		break;
		/*
		case WPT_MARKER:
			{
				float radius	= iconsize/2.0f;;
				float cx		= iconx + radius;
				float cy		= icony + radius;

				ctx->SetColor(0,1,0,1);
				ctx->MovePenPosition(cx, cy);
				ctx->DrawCircle(radius, true);
				ctx->MovePenPosition(-cx, -cy);
			}
		break;
		*/
        default:
            // Invalid type, nothing to draw
            break;
	}

	if(texid > -1)
	{
		ctx->SetTexture(texid);
		phui::draw_gl_quad(box.verts, uv);
	}

	if(_name.size() > 0 && drawName)
	{
		if(_waypoint.handle() == PlayerClient()->ActiveWaypointHandle())
			ctx->SetColor(0,1,0);
		else
			ctx->SetColor(1,1,1);

		ctx->PushState();
		glTranslatef(iconx + iconsize, icony, 0);
		glScalef(1/iconscale, 1/iconscale,1);
		ctx->DrawString(mv->GetFont(), _name.c_str());
		ctx->PopState();
	}

	ctx->SetColor(1,1,1);
}

void WaypointMapObject::Select(SuperCellMapView*)
{
}

void WaypointMapObject::Deselect(SuperCellMapView*)
{
}

bool WaypointMapObject::IsHit(SuperCellMapView* mv, float x, float y) const
{
	float iconscale = mv->Width() / mv->Scale();
	float bottom = _waypoint.y() / SDM_SUPERCELL_SIZE - ICONSIZE/iconscale/2.0f;
	float left = _waypoint.x() / SDM_SUPERCELL_SIZE - ICONSIZE/iconscale/2.0f;
	float top = bottom + ICONSIZE/iconscale;
	float right = left + ICONSIZE/iconscale;

	return (x >= left && x <= right && y >= bottom && y <= top);
}

void WaypointMapObject::Over(SuperCellMapView* mv, int state)
{
	bool wasover = _mouseover;

	_mouseover = (state != phui::MOUSE_EXIT);

	if(wasover != _mouseover)
	{
		if(_mouseover)
		{
			std::wstring tip;

			if(_waypoint.type() == WPT_ENEMY)
			{
				UINT32 duration = teulTime() - _waypoint.posted();
				wchar_t posted[32];
				bool seconds = true;

				tip = uiLocString("waypoints.reportedby");
				tip += L" ";
				tip += chatGetWideCallsign(_waypoint.playerHandle());
				tip += L" ";

				if(duration > 120)
				{
					swprintf(posted, 31, L"%u", (UINT32)(duration/60));
					seconds = false;
				}
				else
				{
					swprintf(posted, 31, L"%u", (UINT32)duration);
				}

				tip += posted;
				tip += L" ";
				tip += uiLocString((seconds) ? "waypoints.secondsago": "waypoints.minutesago");

				if(strlen(_waypoint.notes()))
				{
					tip += L" - ";
					tip += uiUTF8toUTF16(_waypoint.notes());
				}
			}
			else if(_waypoint.type() == WPT_MOBILESPAWN)
			{
				tip = uiLocString("waypoints.mspawn.tip");
			}
			else
			{
				tip += uiUTF8toUTF16(_waypoint.notes());
			}



			// Build Distance mouseover like  "  (Distance: ?m)"
			if(uiInGame())
			{
                // Calculate distance between waypoint and player
#if platOS != OS_MACOS
				char dbuff[8];
#endif
				double dist = -1.0;
				vehVehicle* player = persGetPersonaVehiclePtr();
				if(player != nullptr)
				dist = player->DistanceTo((double)_waypoint.x(), (double)_waypoint.y(), player->ModelData->VehiclePos.z);

				// Build mouseover distance string
				tip += uiUTF8toUTF16("  (");
				tip += uiLocString("common.map.wpdistance");
				tip += uiUTF8toUTF16(": ");
	#if platOS == OS_MACOS
				{
					char abuffer[16];
					strprintf( abuffer, "%d", (int)dist );
					tip += uiUTF8toUTF16(abuffer);
				}
	#else
				tip += uiUTF8toUTF16(_itoa((int)dist, dbuff, 10));
	#endif
				tip += uiUTF8toUTF16(" m)");
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
