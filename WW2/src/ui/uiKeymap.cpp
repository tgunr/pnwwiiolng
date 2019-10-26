#include "syscfg.h" // Path is project specific.
#include <string>
#include <map>
#include <string.h>

#include <libxml/tree.h>

#include "phui/Caption.h"
#include "phui/Periodical.h"
#include "phui/GraphicsContext.h"
#include "phui/Util.h"

#include "chat/chatClientMessages.h"
#include "fio/fio.h"
#include "fp/fpFPS.h"
#include "gl/glplat.h"
#include "imaplib/imapTypes.h"
#include "imap/ww2func.h"
#include "imap/ww2imap.h"
#include "imap/iac.h"
#include "js/jsGame.h"
#include "js/jsKeymap.h"
#include "pers/persmain.h"
#include "tut/TutorialManager.h"
#include "ui/uiMain.h"
#include "ui/uiKeymap.h"

/************************************************************************************************
	uiKeymap
*************************************************************************************************/

bool _ingamekeymapperon	= false;

class ImapPeriodical : public phui::Periodical
{
public:
	void StartInput(int c, int v, unsigned int f, int k, int i, int fc);
	void StopInput();
	virtual void Execute(unsigned int t);

protected:
	std::map<unsigned int, std::pair<int, int> > input;
	int category,  vehidx, keyidx;
	unsigned int functionCode;
	int isaxis;
	imap_keymap keymap;
	bool firstframe;
};

class KeymapView : public phui::Caption
{
public:
	KeymapView(int category, int vehidx, int funcidx, int keyidx);
	virtual bool MakeFocus(bool focus=true);
	virtual void DrawSelf();
	virtual bool MouseUp(float,float,int);

protected:
	int category, vehidx, funcidx, keyidx, depth;
	unsigned int functionCode;
	int isaxis;
	bool firstclick;
};

class JoystickCalibView : public phui::View
{
public:
	JoystickCalibView();
	virtual void DrawSelf();
	virtual bool MouseDrag (float x, float y, int button, bool start);
	bool dragdead, dragfull, dragcontrol;
};

extern imap_queue_entry* imapqueue;
extern unsigned int imapqueuesize;

static ImapPeriodical* imapperiodical;

/************************************************************************************************
	KeymapView
*************************************************************************************************/

KeymapView::KeymapView(int c, int v, int f, int k)
	: phui::Caption()
	, category(c), vehidx(v), funcidx(f), keyidx(k), depth(0), firstclick(true)
{
	SetAcceptsFocus(true);
	functionCode = ww2funcGetCode(category, vehidx, funcidx);
	isaxis = ww2funcIsAxis(functionCode);
}

/************************************************************************************************
	MakeFocus
*************************************************************************************************/

bool KeymapView::MakeFocus(bool focus)
{
	if(!imapperiodical) imapperiodical = new ImapPeriodical;

	if(focus)
		SetBackgroundColor(1,1,1,1);
	else
		SetBackgroundColor(0,0,0,0);

	if(!focus)
		firstclick = true;

	return Caption::MakeFocus(focus);
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

#define MAX_LOOKUP 127

void KeymapView::DrawSelf()
{
	char lookup[MAX_LOOKUP+1];
	imap_keymap keymap{};

	// make sure we're starting from scratch
	*lookup = 0;

	if(isaxis && keyidx == 0)
	{
		if(ww2KeymapGet(category, vehidx,functionCode, keyidx, IMAP_AXIS_KEYMAP, &keymap,&depth) == 0)
		{
			if(keymap.axis.iac != IAC_NOKEY)
				iacPrettyName(keymap.axis.iac, lookup, MAX_LOOKUP);
		}

		std::wstring widename = (keymap.axis.invert) ? L"!" : L"";

		if(keymap.axis.range == -1)
			widename += L"-";
		else if(keymap.axis.range == 1)
			widename += L"+";

		widename += uiUTF8toUTF16(lookup);

		// set our name
		SetValue(widename.c_str());
	}
	else
	{
		std::wstring widename = L"";

		if(!isaxis)
		{
			if(ww2KeymapGet(category, vehidx, functionCode, keyidx, IMAP_BUTTON_KEYMAP, &keymap,&depth) == 0)
			{
				if(keymap.btn.iac[0] != IAC_NOKEY)
				{
					for(int i = 0; i < keymap.numKeys; i++)
					{
						if(i != 0)
							widename += L", ";

						iacPrettyName(keymap.btn.iac[i], lookup, MAX_LOOKUP);
						widename += uiUTF8toUTF16(lookup);
					}
				}
			}

			SetValue(widename.c_str());
		}
		else
		{
			widename = (keyidx == 1) ? L"-: " : L"+: ";

			int realidx = ww2KeymapForIdiotsGetIndex(functionCode, keyidx);
			int keymapType = ww2KeymapForIdiotsGetType(functionCode, keyidx);

			if(ww2KeymapGet(category, vehidx, functionCode, realidx, keymapType, &keymap, &depth) == 0)
			{
				if(keymapType == IMAP_AXIS_REL_KEYMAP && keymap.axisRel.iac[0] != IAC_NOKEY)
				{
					for(int i = 0; i < keymap.numKeys; i++)
					{
						if(i != 0)
							widename += L", ";

						iacPrettyName(keymap.axisRel.iac[i], lookup, MAX_LOOKUP);

						widename += uiUTF8toUTF16(lookup);
					}
				}
				else if(keymapType == IMAP_AXIS_ABS_KEYMAP &&  keymap.axisAbs.iac[0] != IAC_NOKEY)
				{
					for(int i = 0; i < keymap.numKeys; i++)
					{
						if(i != 0)
							widename += L", ";

						iacPrettyName(keymap.axisAbs.iac[i], lookup, MAX_LOOKUP);

						widename += uiUTF8toUTF16(lookup);
					}
				}
			}

			SetValue(widename.c_str());
		}
	}

	if(IsFocus())
	{
		SetTextColor(0,0,0);
	}
	else
	{
		switch(depth)
		{
			case 1:
				SetTextColor(1,1,0);
			break;
			case 2:
				SetTextColor(0,1,0);
			break;
			default:
				SetTextColor(1,1,1);
			break;
		}
	}

	Caption::DrawSelf();
}

/************************************************************************************************
	MouseUp
*************************************************************************************************/

bool KeymapView::MouseUp(float x,float y,int button)
{
	if(IsFocus())
	{
		if(firstclick)
		{
			firstclick = false;
		}else
		{
			imapperiodical->StartInput(category, vehidx, functionCode, keyidx, isaxis, funcidx);
			firstclick = true;
		}
	}
	return true;
}

/************************************************************************************************
	uiKeymapCreate
*************************************************************************************************/

phui::View* uiKeymapCreate(_xmlNode* xmlnode, phui::View* parent)
{
	xmlChar* val;
	int category, vehidx, funcidx, keyidx;
	category = vehidx = funcidx = keyidx = 0;

	val = xmlGetProp(xmlnode, (xmlChar*)"category");
	if(val)
	{
		category = atoi((char*)val);
		xmlFree(val);
	}
	val = xmlGetProp(xmlnode, (xmlChar*)"vehidx");
	if(val)
	{
		vehidx = atoi((char*)val);
		xmlFree(val);
	}
	val = xmlGetProp(xmlnode, (xmlChar*)"funcidx");
	if(val)
	{
		funcidx = atoi((char*)val);
		xmlFree(val);
	}
	val = xmlGetProp(xmlnode, (xmlChar*)"keyidx");
	if(val)
	{
		keyidx = atoi((char*)val);
		xmlFree(val);
	}

	KeymapView* kv = new KeymapView(category, vehidx, funcidx, keyidx);

	return kv;
}

/************************************************************************************************
	uiJoystickCalibrationCreate
*************************************************************************************************/

phui::View* uiJoystickCalibrationCreate(_xmlNode*, phui::View* parent)
{
	return new JoystickCalibView;
}

/************************************************************************************************
	Execute
*************************************************************************************************/

void ImapPeriodical::Execute(unsigned int t)
{
	if(firstframe)
	{
		firstframe = false;
		return;
	}

	for(unsigned int i = 0; i < imapqueuesize; i++)
	{
		imap_queue_entry const& entry = imapqueue[i];

		if(isaxis && keyidx == 0)
		{
			if(entry.iac == IAC_LEFTCONTROL || entry.iac == IAC_RIGHTCONTROL)
				keymap.axis.invert = entry.state;
			if(entry.iac == IAC_EQUALS || entry.iac == IAC_NUMPADADD)
				keymap.axis.range = entry.state ? 1 : 0;
			if(entry.iac == IAC_MINUS || entry.iac == IAC_NUMPADSUBTRACT)
				keymap.axis.range = entry.state ? -1 : 0;

			if(IAC_IS_AXIS(entry.iac))
			{
				std::map<unsigned int, std::pair<int, int> >::iterator it = input.find(entry.iac);
				if(it == input.end())
				{
					input[entry.iac] = std::pair<int,int>(entry.count, entry.count);
					it = input.find(entry.iac);
				}else
				{
					if(IAC_IS_MOUSE_AXIS(entry.iac))
						it->second.second += entry.count;
					else
						it->second.second = entry.count;
				}

				int limit = 500;

				if(entry.iac == IAC_JOYSTICK_TRACK_IR_Y)
					limit = 75;
				else if(entry.iac == IAC_JOYSTICK_TRACK_IR_X)
					limit = 300;

				if(abs(it->second.first - it->second.second) > limit)
				{
					keymap.numKeys = 1;
					keymap.keymapType = IMAP_AXIS_KEYMAP;
					keymap.axis.iac = imapqueue[i].iac;
					ww2KeymapSet(category, vehidx,functionCode, keyidx, &keymap);
					StopInput();
					uiGame()->RunScript("onkeymapchange()");
					break;
				}
			}else
			{
				if(entry.iac == IAC_ESCAPE)
				{
					StopInput();
					uiGame()->RunScript("onkeymapchange()");
					break;
				}
			}
		}else
		{
			if(!IAC_IS_AXIS(entry.iac))
			{
				std::map<unsigned int, std::pair<int, int> >::iterator it = input.find(entry.iac);
				bool setkeymap = false;
				if(it != input.end() || (input.size() < 2 && entry.state))
				{
					input[entry.iac].first = entry.state;
					setkeymap = true;
				}

				if(setkeymap)
				{
					if(!isaxis)
					{
						it = input.begin();
						keymap.numKeys = 0;
						keymap.keymapType = IMAP_BUTTON_KEYMAP;

						for(; it != input.end(); it++)
						{
							keymap.btn.iac[keymap.numKeys++] = it->first;
						}

						ww2KeymapSet(category, vehidx,functionCode, keyidx, &keymap);

						if(entry.state == 0 || keymap.numKeys == 2)
						{
							StopInput();
							uiGame()->RunScript("onkeymapchange()");
							break;
						}
					}else
					{
						int realidx = ww2KeymapForIdiotsGetIndex(functionCode, keyidx);
						unsigned int flags = ww2funcFlags(functionCode);
						keymap.keymapType = ww2KeymapForIdiotsGetType(functionCode, keyidx);

						if(keymap.keymapType == IMAP_AXIS_REL_KEYMAP)
						{
							it = input.begin();
							keymap.numKeys = 0;
							for(; it != input.end(); it++)
								keymap.axisRel.iac[keymap.numKeys++] = it->first;

							if(flags & WW2_FUNC_KEYMAPFORIDIOTS_AXIS_REL_KP)
							{
								keymap.axisRel.axisMapType = IMAP_AXIS_PER_STROKE;
								keymap.axisRel.rate = (keyidx == 1) ? .01f : -.01f;
							}else
							{
								keymap.axisRel.axisMapType = IMAP_AXIS_PER_SEC;
								keymap.axisRel.rate = (keyidx == 1) ? .1f : -.1f;
							}
						}else
						{
							it = input.begin();
							keymap.numKeys = 0;
							for(; it != input.end(); it++)
								keymap.axisAbs.iac[keymap.numKeys++] = it->first;

							if(flags & WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_40_60)
								keymap.axisAbs.percent = (keyidx == 1) ? .4f : .6f;
							else
								keymap.axisAbs.percent = (keyidx == 1) ? 0.0f : 1.0f;
							// onrelease flag handled internally in ww2imap
						}

						ww2KeymapSet(category, vehidx,functionCode, realidx, &keymap);

						if(entry.state == 0 || keymap.numKeys == 2)
						{
							StopInput();
							uiGame()->RunScript("onkeymapchange()");
							break;
						}
					}
				}
			}
		}
	}
}

/************************************************************************************************
	StartInput
*************************************************************************************************/

void ImapPeriodical::StartInput(int c, int v, unsigned int f, int k, int i, int fc)
{
	firstframe = true;
	category = c; vehidx = v; functionCode = f; keyidx = k; isaxis = i;
	memset(&keymap, 0x0, sizeof(keymap));

	StartExecuting();
	uiSetCursor(0);

	jsval rval;
	jsval argv[1] = { (new jsKeymap(uiGame()->jsContext(), uiGame()->jsObject(), c, v, fc, keyidx))->jsVal() };

	uiGame()->CallFunction("onkeymapenter", 1, argv, &rval);
}

/************************************************************************************************
	StopInput
*************************************************************************************************/

void ImapPeriodical::StopInput()
{
	StopExecuting();
	input.clear();
	uiSetCursor(1);
}

/************************************************************************************************
	JoystickCalibView
*************************************************************************************************/

JoystickCalibView::JoystickCalibView()
{
	dragdead = dragfull = dragcontrol = false;
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void JoystickCalibView::DrawSelf()
{
	phui::GraphicsContext* ctx = GetGraphicsContext();
	ctx->DisableTexture();

	float deadband, fullband, x, y;
	unsigned int iac = iacCode(GetValue().String());
	ww2KeymapGetJoysickCalibration(iac, &deadband, &fullband, &x, &y);

    float cx = x * Width();
    float cy = y * Height();

	float x1 = deadband * Width();
    float x2 = cx;
    float x3 = x2;
    float x4 = fullband * Width();

    float y1 = 0;
    float y2 = cy;
    float y3 = y2;
    float y4 = Height();

    float delta = 1.0f / 64.0f;
    float t = 0.0f;

	ctx->SetColor(1,1,1);
	glBegin(GL_LINE_STRIP);
    for(int i = 0; i <= 64; i++)
    {
        float x_ = (1 - t) * (1 - t) * (1 - t) * x1 +
			        3 * t * (1 - t) * (1 - t) * x2 +
			        3 * t * t * (1 - t) * x3 +
			        t * t * t * x4;

        float y_ = (1 - t) * (1 - t) * (1 - t) * y1 +
		            3 * t * (1 - t) * (1 - t) * y2 +
		            3 * t * t * (1 - t) * y3 +
		            t * t * t * y4;

		glVertex2f(x_,y_);

        t += delta;
    }
	glEnd();

	phui::BoxVerts box;
    phui::box_verts_with_pos(box,cx-5,cy-5,10,10);

	ctx->SetColor(1,0,0);
    ctx->DrawQuad(box.verts);

    ctx->SetColor(0,1,0);
    phui::box_verts_with_pos(box,x1-2,0,4,Height());
    ctx->DrawQuad(box.verts);
 	phui::box_verts_with_pos(box,x4-2,0,4,Height());
    ctx->DrawQuad(box.verts);

}
/************************************************************************************************
	MouseDrag
*************************************************************************************************/

bool JoystickCalibView::MouseDrag (float dragx, float dragy, int button, bool start)
{
	float deadband, fullband, x, y;
	unsigned int iac = iacCode(GetValue().String());
	ww2KeymapGetJoysickCalibration(iac, &deadband, &fullband, &x, &y);

    float cx = x * Width();
    float cy = y * Height();

	float x1 = deadband * Width();
    float x4 = fullband * Width();

	if(start)
	{
		dragdead = dragfull = dragcontrol = false;

		if(dragx >= x4-10 && dragx <= x4+10)
		{
			dragfull = true;
			_dragx = dragx - x4;
		}

		if(dragx >= x1-10 && dragx <= x1+10)
		{
			dragdead = true;
			_dragx = dragx - x1;
		}

		if(dragx >= cx-10 && dragx <= cx+10 && dragy >= cy-10 && dragy <= cy+10)
		{
			dragcontrol = true;
			_dragx = dragx - cx;
			_dragy = dragy - cy;
		}
	}

	if(!dragdead && !dragfull && !dragcontrol) return true;

	if(dragcontrol)
	{
		cx = dragx - _dragx;
		cy = dragy - _dragy;

		x = cx / Width();
		y = cy / Height();
	}else if(dragdead)
	{
		x1 = dragx - _dragx;
		deadband = x1 / Width();
	}else if(dragfull)
	{
		x4 = dragx - _dragx;
		fullband = x4 / Width();
	}

	if(deadband > fullband - .1f) deadband = fullband - .1f;
	if(deadband < 0) deadband = 0;
	if(deadband > 1) deadband = 1;

	if(fullband < deadband + .1f) fullband = deadband + .1f;
	if(fullband < 0) fullband = 0;
	if(fullband > 1) fullband = 1;

	if(x < deadband + .05f) x = deadband + .05f;
	if(x > fullband - .05f) x = fullband - .05f;
	if(x < 0) x = 0;
	if(x > 1) x = 1;
	if(y < 0) y = 0;
	if(y > 1) y = 1;

	ww2KeymapSetJoysickCalibration(iac, deadband, fullband, x, y);

	return true;
}

/************************************************************************************************
	uiMapToggleIngameKeymapper
*************************************************************************************************/

UINT32 uiMapToggleIngameKeymapper(SINT32 keyState,  UINT32 functionNum, UINT32 data)
{
	if(!keyState)
		return 0;

	_ingamekeymapperon = !_ingamekeymapperon;

	if(persIsCrewDead())
		_ingamekeymapperon = false;

	phui::View* _ingamekeymapper = uiViewport()->FindView("ingamekeymapper");

	if(_ingamekeymapperon)
	{
		if(!_ingamekeymapper)
			_ingamekeymapper = uiLoadView("ingamekeymapper", true, false);

		fpFPSMapState(TRUE);
		uiSetCursor(1);

		_ingamekeymapper->Show();
	}
	else
	{
		fpFPSMapState(FALSE);

		if(_ingamekeymapper)
			uiMapHideIngameKeymapper();
	}

	uiGC();

	return 0;
}

/************************************************************************************************
	uiMapHideIngameKeymapper
*************************************************************************************************/

void uiMapHideIngameKeymapper(bool close)
{
	phui::View* _ingamekeymapper	= uiViewport()->FindView("ingamekeymapper");

	if(_ingamekeymapper)
	{
		_ingamekeymapper->Hide();

		uiSetCursor(0);

		if(close || fioQuery() == ISDIRECTORY)
		{
			_ingamekeymapper->Close();
			_ingamekeymapper = nullptr;
		}
	}

	uiGC();

	_ingamekeymapperon = false;
}
