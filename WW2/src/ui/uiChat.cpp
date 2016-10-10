#include <syscfg.h> // Path is project specific.
#include <stdarg.h>
#include <math.h>
#include <deque>

#include <libxml/tree.h>
#include <regex>
#define RENS std

#include "phui/KeyCodes.h"
#include "phui/Control.h"
#include "phui/GraphicsContext.h"
#include "phui/ListView.h"
#include "phui/Caption.h"
#include "phui/TextView.h"
#include "phui/TextControl.h"
#include "phui/Scalar.h"
#include "js/jsClient.h"

#include "ui/uiMain.h"
#include "ui/uiChat.h"
#include "imap/iac.h"
#include "chat/chatHandle.h"
#include "fio/fio.h"
#include "locl/loclLang.h"
#include "draw/drawIcon.h"
#include "client/GameClient.h"
#include "pers/pershud.h"
#include "map/mapPersonaSummary.h"
#include "miss/missionDetail.h"
#include "chat/chatBrigadeControlList.h"
#include "ui/uiDotCommands.h"
#include "ui/uiGameEvents.h"
#include "xml/XMLLoader2.h"
#include "chat/chatGCIMessage.h"
#include "chat/chatHostMessage.h"
#include "chat/chatLocalizedMessage.h"
#include "chat/chatTuneToChannel.h"
#include "chat/chatUserNLA.h"
#include "ui/uiSound.h"
#include "tut/TutorialManager.h"
#include "chat/chatClientMessages.h"

#include "cfg/cfgPrefs.h"

#ifdef GR2_BLEND_EDITOR
#include "gr2/gr2Anim.h"
#endif


/******** DEFINES ********/

#define MSG_COLOR colorWHITEALPHA
#define MAX_CHAT_LINES 256
#define MAGIC_DOT_R_COUNTRY 15

// previous text limit was 250, account for encoded size and header/trailer characters
#define MAX_REPORT_LEN (250 - INFO_MAX_ENCODE_SIZE - 2)

/******** OBCENITY FILTER TYPES ********/

// Structure that houses an obscenity filter rule.
using OBSCENITY_RULE = RENS::wregex;

// Vector of rules forms a filter.
using OBSCENITY_FILTER = std::vector<OBSCENITY_RULE>;

// word filter loader
class BadWordLoader : public XMLLoader2<BadWordLoader>
{
public:

	BadWordLoader(const char* path) : XMLLoader2<BadWordLoader>(path)
	{
		AddHandler("rule", &BadWordLoader::DoRule, this);
	}

	virtual ~BadWordLoader()
	{
	}

	// tag handlers
	XML_TAG_HANDLER(DoRule)
	{
		if(first && userData)
		{
			OBSCENITY_FILTER* filter = (OBSCENITY_FILTER*)userData;
			OBSCENITY_RULE rule;

			const wchar_t* definition = uiUTF8toUTF16(xmlGetStringFromNode(node));
			bool assigned = false;

			try
			{
				 // Set up the regular expression for case-insensitivity
				 rule.assign(definition, RENS::regex_constants::icase);
				 assigned = true;
			}
			catch (RENS::regex_error& /* e */)
			{
				// not a valid regular expression
			}

			if(assigned)
				filter->push_back(rule);
		}

		return true;
	}
};

/******** LOCAL VARIABLES ********/

static phui::View* _chatwin;
static phui::ListView* _chatview;
static phui::Control* _chatmsg;
static phui::Control* _chatsay;
ChatChannel ChatChannels[MAX_CLIENT_CHANNELS];
std::wstring lastplayerhandle;

static int uiChatPrintLinev(int* icons, unsigned int color, const wchar_t* format, va_list& marker);
static drawIconRef _commicons;
static phui::TextHistory* _chatHistorySave = nullptr; // used to save history of chat window resets
static OBSCENITY_FILTER _wordFilter;

struct ChatLine
{
	ChatLine(){};
	ChatLine(int* i, unsigned int c, const wchar_t* str) : color(c), line(str)
	{
		if(i)
			memcpy(icons, i, CHATICON_MAX*sizeof(int));
		else
			icons[0] = icons[1] = icons[2] = -1;
	}
	int icons[CHATICON_MAX];
	unsigned int color;
	std::wstring line;
};

static std::deque<ChatLine> _chatlines;

class ChatInput : public phui::TextControl
{
public:
	virtual bool KeyDown(int keycode, unsigned short unicode);
};

static void uiChatAddLine(int* icons, unsigned int color, const wchar_t* buf);

/************************************************************************************************
	uiChatCreate
*************************************************************************************************/

phui::View* uiChatCreate(_xmlNode*, phui::View* parent)
{
	_chatview = new phui::ListView;

	if(!_commicons)
		_commicons = drawIconLoad("ui/uidata/chat/commicons3");

	return _chatview;
}

/************************************************************************************************
	uiChatInit
*************************************************************************************************/

void uiChatInit()
{
	if(Online)
	{
		uiChatInitWordFilter();
		setChatRateLimiting();
	}
}

/************************************************************************************************
	uiChatKill
*************************************************************************************************/

void uiChatKill()
{
    if(_commicons)
    {
        drawIconKill(_commicons);
        _commicons = nullptr;
    }
}

/************************************************************************************************
	uiChatPrintLine
*************************************************************************************************/

int uiChatPrintLine(const char* format, ...)
{
 	if(!_chatview) return -1;

	static char buffer[MAX_PRINTABLE_LENGTH+1];

	va_list marker;
	va_start(marker, format);

	// since we're passing in a char* string, we'll do our own varargs formatting
	int len = vsnprintf(buffer, MAX_PRINTABLE_LENGTH, format, marker);

	// make sure the string is null-terminated
	if(len > 0 && len <= MAX_PRINTABLE_LENGTH)
		buffer[len] = 0;

	// pass on down to chat
	len = uiChatPrintLinev(nullptr, MSG_COLOR, uiUTF8toUTF16(buffer), marker);

	va_end(marker);

	return len;
}

int uiChatPrintLine(const wchar_t* format, ...)
{
 	if(!_chatview) return -1;

	va_list marker;
	va_start(marker, format);
	int len = uiChatPrintLinev(nullptr,MSG_COLOR, format, marker);
	va_end(marker);

	return len;
}

/************************************************************************************************
	uiChatPrintUILocLine

	Prints a colored localized line from ui.dtd. Handles up to 5 dynamic variable
	strings within the loc string.

*************************************************************************************************/

void uiChatPrintUILocLine(int icons[CHATICON_MAX], unsigned int color, const char* key, ...)
{
 	if(!_chatview) return;

	int index = -1;
	wchar_t placeHolder[4]; // placeholders are in the form {0}, {1}, {n}... in ui.dtd
	wchar_t* arg = nullptr;

	// the format string
	std::wstring format = uiLocString(key);

	// init arrays
	placeHolder[0] = '\0';

	// we'll search for up to 6 dynamic variables in the localized message

	va_list marker;
	va_start( marker, key ); /* Initialize variable arguments. */

	for(int p = 1; p < 7; ++p) // loc vars are 1 based
	{
		arg = va_arg( marker, wchar_t*);

		if(arg)
		{
			// setup which placeholder we're converting
			swprintf(placeHolder, 4, L"{%d}", p);

			while((index = format.find(placeHolder)) >= 0)
				format.replace(index, 3, arg);
		}
		else
			break; // reached null
	}

	va_start( marker, key ); /* Initialize variable arguments. */
	uiChatPrintLinev(icons, color, format.c_str(), marker);
	va_end( marker ); /* Reset variable arguments. */
}

/************************************************************************************************
	uiChatPrintLineColor
*************************************************************************************************/

void uiChatPrintLineColor(unsigned int color, const wchar_t* format, ...)
{
 	if(!_chatview) return;

	va_list marker;
	va_start(marker, format);
	uiChatPrintLinev(nullptr,color, format, marker);
	va_end(marker);
}

/************************************************************************************************
	uiChatPrintLineIcon
*************************************************************************************************/

void uiChatPrintLineIcon(int icons[CHATICON_MAX], unsigned int color, const wchar_t* format, ...)
{
 	if(!_chatview) return;

	va_list marker;
	va_start(marker, format);
	uiChatPrintLinev(icons, color, format, marker);
	va_end(marker);
}

/************************************************************************************************
	uiChatPrintLineFeedback
*************************************************************************************************/

void uiChatPrintLineFeedback(int locgroup, int locid, bool iserror, ...)
{
	const char* category = (const char*)loclGetStringCategory((ENUM_CATEGORIES)locgroup);
	char lockey[128+1];

	// form the key
	snprintf(lockey, 128, "loc.%s.%d", category, locid);

	// lookup the message
	const std::wstring& format = uiLocString(lockey);

	unsigned int color;
	int icons[CHATICON_MAX] = {-1,-1,-1};

	icons[0] = (iserror) ? IDE_ICON_CAUTION : IDE_ICON_NOTE;
	color =  (iserror) ? colorTEXTCAUTION : colorTEXTNOTE;

	va_list marker;
	va_start(marker, iserror);
	uiChatPrintLinev(icons,color, format.c_str(), marker);
	va_end(marker);
}

/************************************************************************************************
  _lstringToText (may return nullptr)
* helper function for uiChatPrintLineMessageLoc to convert lstring IDs into text* OR nullptr*
************************************************************************************************/

static inline const wchar_t* _lstringToText(ENUM_CATEGORIES loc, int message
											 , const wchar_t* const arg1, const wchar_t* const arg2, const wchar_t* const arg3, const wchar_t* const arg4, const wchar_t* const arg5, const wchar_t* const arg6)
{
	const char* lockey = (char*)loclLangGetString(loc, message);

	if ( loc != IDE_LANG_HOST || message < LSTR_TEXT_START || message > LSTR_TEXT_END )
	{
		static std::wstring output;
		output = uiLocString(lockey).c_str();

		const wchar_t* const args[Chat::LocalizedMessage::_NUM_ARGUMENTS] = { arg1, arg2, arg3, arg4, arg5, arg6 };
		wchar_t format[5];
		UINT32 pos;

		for ( UINT32 i = 0; i < Chat::LocalizedMessage::_NUM_ARGUMENTS; ++i )
		{
			// Other parts of the DTD still use %s.
			if ( loc != IDE_LANG_HOST )
			{
				pos = output.find(L"%ls");
				if ( pos != output.npos )
					output.replace(pos, 3, (args[i][0] ? args[i] : L"?null?"));
			}

			swprintf(format, 5, L"{%1u}", i + 1);

			while ( (pos = output.find(format)) != output.npos )
			{
				output.replace(pos, 3, (args[i][0] ? args[i] : L"?null?"));
			}
		}

		return output.c_str();
	}

	static std::wstring stringBuf;

	// We'll need to track when stringBuf has been finished and needs erasing
	static bool clearStringBuf = false;

	if(clearStringBuf)
	{
		stringBuf.clear();
		clearStringBuf = false;
	}

	if ( message == LSTR_TEXT_START )
	{
		// Start a new long-string message
		stringBuf = arg1;
		return nullptr;
	}
	else if ( message == LSTR_TEXT_LINE )
	{
		if(stringBuf.length() > 0)
			stringBuf.append(L"\n");
	}
	else if(stringBuf.length() > 0)
		stringBuf.append(L" ");

	stringBuf.append(arg1);

	if ( message != LSTR_TEXT_END )
		return nullptr;

	clearStringBuf = true;
	return stringBuf.c_str();
}

// Play a buzzer sound.

static void _buzzer()
{
	static int nextPlayWindow = 0;
	if ( ft1000mscount >= nextPlayWindow )
	{
#ifdef ORIGINALLY
		uiPlaySound("sounds/buzzer.wav", true, 0.50, nullptr);
#else
		uiPlaySound("sounds/Drum1.wav", true, 0.48, nullptr);
#endif
		nextPlayWindow = ft1000mscount + 6;
	}
}

/************************************************************************************************
	uiChatPrintLineMessageLoc
	Note: "loc" is ENUM_CATEGORIES
************************************************************************************************/

void uiChatPrintLineMessageLoc(int loc, int prefix, int message
								, const wchar_t* const arg1, const wchar_t* const arg2, const wchar_t* const arg3, const wchar_t* const arg4, const wchar_t* const arg5, const wchar_t* const arg6)
{
	if(prefix == LSTR_BETA && gRegion == 5) // no beta messages for chinese, requested by peter
		return;

	unsigned int color = colorGREENALPHA;	// Default color
	int side = 0;
	int icons[CHATICON_MAX] = {-1,-1,-1};
	wchar_t prefixstr[32];
	*prefixstr = 0;

	switch (prefix)
	{
		case LSTR_MOTD:
		case LSTR_SYSTEM:
			color = colorYELLOWALPHA;
			icons[2] = IDE_ICON_NOTE;
		break;

		case LSTR_AXIS_HC:
		case LSTR_ALLIED_HC:
			side = (prefix == LSTR_AXIS_HC) ? AXIS : ALLIED;
			icons[2] = IDE_ICON_HC;
			prefix = LSTR_NONE;
			color = colorCYANALPHA;
		break;

		case LSTR_HQ:
			prefix = LSTR_NONE;
			icons[0] = IDE_ICON_GCI;
			color = colorCYANALPHA;
		break;

		case LSTR_WARNING:
			_buzzer();
		case LSTR_REPORT:
			color = colorREDALPHA;
		break;

		case LSTR_NOTE:
			prefix = LSTR_NONE;
			if(message == LSTR_AFK)
				uiSetAFK(frametime);
			else if(message == LSTR_NOT_AFK)
				uiSetAFK(0);
		case LSTR_BETA:
			icons[2] = IDE_ICON_NOTE;
			color = colorGREENALPHA;
		break;

		case LSTR_MISSION_POST_DENIED:
		// PASSTHRU
		case LSTR_ERROR:
		// PASSTHRU
		case LSTR_CAUTION:
			prefix = LSTR_NONE;
			icons[2] = IDE_ICON_CAUTION;
			color = colorORANGEALPHA;
		break;

		case LSTR_ALLIED:
		case LSTR_AXIS:
			side = (prefix == LSTR_AXIS) ? AXIS : ALLIED;
			icons[0] = IDE_ICON_GCI;
			icons[2] = IDE_ICON_HC;
			color = MSG_COLOR;
		break;

		case LSTR_SQUAD:
			prefix = LSTR_NONE;
			icons[2] = IDE_ICON_SQUAD;
			color = colorTEXTSQUAD;
		break;

		case LSTR_MISSION:
			prefix = LSTR_NONE;
			icons[2] = IDE_ICON_MISSION;
			color = colorTEXTNOTE;
			//if(message == LSTR_ORDERS)
		break;

		default:
		break;
	}

	if ( side > 0 )
		icons[1] = (side == ALLIED) ? IDE_ICON_ALLIED : IDE_ICON_AXIS;

	const wchar_t* textPtr = _lstringToText((ENUM_CATEGORIES)loc, message, arg1, arg2, arg3, arg4, arg5, arg6);
	if ( textPtr == nullptr )
		return;

	// we don't handle generic orders because thats handled in GameClient
	if(prefix == LSTR_ORDERS && message != LSTR_GENERIC)
		PlayerClient()->SetCurrentOrders(textPtr);

	if (prefix != LSTR_NONE)
		swprintf(prefixstr, 31, L"%ls: ", uiLocString(IDE_LANG_HOST, prefix).c_str());

	uiChatPrintLineIcon(icons, color, L"%ls%ls", prefixstr, textPtr);
}

/************************************************************************************************
	uiChatPrintLinev
*************************************************************************************************/

int uiChatPrintLinev(int* icons, unsigned int color, const wchar_t* format, va_list& marker)
{
 	if(!_chatview) return -1;

	int numrows = (int)_chatview->NumRows();

	if(numrows >= MAX_CHAT_LINES)
	{
		_chatview->ScrollBy(0,-_chatview->GetRowView(0)->Height());
		_chatview->RemoveRow(0);
		_chatlines.pop_front();
	}

	static wchar_t buf[MAX_PRINTABLE_LENGTH+1];
	int len = vswprintf(buf, MAX_PRINTABLE_LENGTH, format, marker);
	if(len > 0 && len <= MAX_PRINTABLE_LENGTH)
		buf[len] = 0;

	_chatlines.push_back(ChatLine(icons, color, buf));

	uiChatAddLine(icons, color, buf);

	return len;
}

/************************************************************************************************
	uiChatAddLine
*************************************************************************************************/

static void uiChatAddLine(int* icons, unsigned int color, const wchar_t* buf)
{
	phui::GraphicsContext* ctx = _chatview->GetGraphicsContext();

	float x = 0;
	float height = 	ctx->FontHeight(_chatview->GetFont());
	float r = piRGBA_GETRED(color)/255.0f;
	float g = piRGBA_GETGREEN(color)/255.0f;
	float b = piRGBA_GETBLUE(color)/255.0f;
	float a = piRGBA_GETALPHA(color)/255.0f;

	// lets get the margins
	float chatwidth = _chatview->Width();
	float lm, rm, tm, bm;

	_chatview->GetMargins(&lm, &rm, &tm, &bm);

	// lets calculate the width of the chatview once
	chatwidth -= rm;

	phui::View* row = new phui::View;
	row->ResizeTo(chatwidth, height);

	if(icons)
	{
		for(int i = 0; i < CHATICON_MAX; i++)
		{
			if(icons[i] > -1)
			{
				DrawIconView* iconview = new DrawIconView(_commicons, icons[i]);
				iconview->ResizeTo(height, height);
				iconview->MoveTo(x, 0);
				iconview->SetResizeMode(phui::View::FOLLOW_LEFT | phui::View::FOLLOW_TOP);
				row->AddChild(iconview);

			}
			x += height;
		}
	}else
	{
		x = CHATICON_MAX * height;
	}

	phui::TextView* textview = new phui::TextView;
	_chatview->AddChild(textview); // need a context

	textview->SetLineSpacing(0);
	textview->SetMouseScroll(false);
	textview->SetAcceptsFocus(false);
	textview->SetFont(_chatview->GetFont());
	textview->ResizeTo(chatwidth - x, height);
	textview->SetText(buf);
	textview->SetTextColor(r,g,b,a);
	textview->SetResizeMode(phui::View::FOLLOW_LEFT_RIGHT);
	textview->MoveTo(x,0);
	height *= textview->NumLines();
	textview->ResizeTo(chatwidth - x, height);
	row->ResizeTo(chatwidth, height);

	_chatview->RemoveChild(textview);
	row->AddChild(textview);
	_chatview->AddRow(row);

	if( (_chatview->ContentHeight() > _chatview->Height()) &&
		(_chatview->ScrollY() > (_chatview->ContentHeight() - _chatview->Height() - 2*height)) )
	{
		_chatview->ScrollBy(0,height);
	}
}

/************************************************************************************************
	uiChatEnterFocus
*************************************************************************************************/

void uiChatInvoke()
{
	if(!uiChatOK())
		uiChatShow();

	if(_chatsay)
		_chatsay->Invoke();
}

/************************************************************************************************
	uiChatOK
*************************************************************************************************/

bool uiChatOK()
{
	return (_chatwin && _chatwin->IsVisible());
}

/************************************************************************************************
	uiChatShow
*************************************************************************************************/

void uiChatShow()
{
	if(!_chatwin)
	{
		_chatwin = uiLoadView("chat");

		if(_chatwin)
		{
			_chatsay	=  dynamic_cast<phui::Control*>(_chatwin->FindView("chatsay"));
			_chatmsg	=  dynamic_cast<phui::Control*>(_chatwin->FindView("chatmsg"));

			std::deque<ChatLine>::iterator it = _chatlines.begin();
			for(; it != _chatlines.end(); ++it)
				uiChatAddLine(it->icons, it->color, it->line.c_str());

			// reset chat's history if we had any
			if(_chatHistorySave)
			{
				phui::TextControl* input =  dynamic_cast<phui::TextControl*>(_chatmsg);
				if(input) input->SetHistory(_chatHistorySave, true);

				// kill our saved history
				_chatHistorySave->clear();
				delete _chatHistorySave;
				_chatHistorySave = nullptr;
			}
		}
	}

	if(_chatwin && !_chatwin->IsVisible())
		_chatwin->Show();
}

/************************************************************************************************
	uiChatHide
*************************************************************************************************/

void uiChatHide(bool close)
{
	if(_chatwin)
	{
		_chatwin->Hide();

		if(close)// || fioQuery() == ISDIRECTORY)
		{
			if(_chatmsg)
			{
				phui::TextControl* input = dynamic_cast<phui::TextControl*>(_chatmsg);

				if(input)
				{
					// lets save our chat history
					_chatHistorySave = new phui::TextHistory;
					input->GetHistory(_chatHistorySave);
					input->SetText("");
					input->SetSelection(0, 0);
				}
			}

			_chatwin->Close();
			_chatwin = nullptr;
			_chatview = nullptr;
			_chatmsg = nullptr;
			_chatsay = nullptr;
		}
	}
}

/************************************************************************************************
	uiChatSyncToSettings
*************************************************************************************************/

void uiChatSyncToCycleSetting()
{
	// can only turn chat off in game, not in the UI
	if(HUDVisible == HUD_CYCLE_OFF && uiInGame())
		uiChatHide();
	else
		uiChatShow();
}

/************************************************************************************************
	uiChatToggleHeight
*************************************************************************************************/

UINT32 uiChatToggleHeight(SINT32 param, UINT32 functionNum, UINT32 alwaysZero)
{
	if(_chatwin && !param)
	{
		phui::DOM::ElementDOM* viewDom = dynamic_cast<phui::DOM::ElementDOM*>(_chatwin->UserData(DOM_VIEW_USERDATA));

		if(viewDom)
			viewDom->RunScript("toggleChatHeight()");
	}

	return 0;
}

/************************************************************************************************
	uiChatInput
*************************************************************************************************/

bool uiChatInput(unsigned int iac)
{
	bool handled		= false;
	bool action			= (iac == IAC_ENTER || iac == IAC_F1 || iac == IAC_F2 || iac == IAC_F3
					                        || iac == IAC_F4 || iac == IAC_F5 || iac == IAC_F6);

	/*
	 * if your in game, hitting an action key (enter or F keys) will
	 * bring up chat and let you send a message
	 * once the message is sent, the chat window hides
	 * based on the hud cycle setting in the js layer
	*/
	if(action && uiInGame())
		uiChatShow();

	// UI controls when chat is available
	if(uiChatOK())
	{
		if(action)
		{
			if(!uiFocused())
			{
				// area chat is only invoked if enter is the first input key pressed
				PlayerClient()->SetAreaChat(iac == IAC_ENTER);
				uiChatInvoke();
			}

			if(iac != IAC_ENTER)
			{
				ChatChannels[iac - IAC_F1].xmit = !ChatChannels[iac - IAC_F1].xmit;
				handled = true; // aw - fix for [] appearing in chat data
			}
		}
		else if(iac == IAC_ESCAPE)
		{
			if(_chatmsg && _chatmsg->IsFocus())
			{
				// ui is constantly keeping the presentation layer in sync with ChatChannels
				// so on escape, we need to clear the xmit flag
				for(int i = 0; i < MAX_CLIENT_CHANNELS; i++)
					ChatChannels[i].xmit = false;

				_chatmsg->SetValue("");

				uiChatInvoke();
				uiRunScript("game.onchatdeactivate()"); // sync the chat window to the hud cycle setting

#ifdef GR2_BLEND_EDITOR
				if(getEditModel())
					uiChatHide();
#endif
			}
		}
	}

	return handled;
}

/************************************************************************************************
	uiChatTuneToChannel
*************************************************************************************************/

void uiChatTuneToChannel(Chat::TuneToChannel const& tunechannel)
{
	if(tunechannel.m_radioIndex >= MAX_CLIENT_CHANNELS) return;

	ChatChannel& channel	= ChatChannels[tunechannel.m_radioIndex];
	channel.type			= (int)tunechannel.m_type;
	channel.recv			= (int)tunechannel.m_rx;
	channel.xmit			= (int)tunechannel.m_tx;
	channel.name			= L"custom";

	wchar_t buffer[10+1];

	if(chatGetCallsign(buffer, tunechannel.m_handle))
	{
		channel.name = buffer;
	}
	else
	{
		switch(tunechannel.m_type)
		{
			case BRIGADE_CHANNEL:
				channel.name = L"brigade";
			break;
			case MISSION_CHANNEL:
				channel.name = L"mission";
			break;
			case SQUAD_CHANNEL:
				channel.name = L"squad";
			break;
			case ORIGIN_CP_CHANNEL:
				channel.name = L"origin";
			break;
			case OBJECTIVE_CP_CHANNEL:
				channel.name = L"objective";
			break;
			case SIDE_GENERAL_CHANNEL:
				channel.name = L"side";
			break;
			case SIDE_HELP_CHANNEL:
				channel.name = L"help";
			break;
			case SIDE_OPS_CHANNEL:
				channel.name = L"ops";
			break;
			default:
				// do nothing
			break;
		}
	}
}

/************************************************************************************************
	uiChatDetuneChannel
*************************************************************************************************/

void uiChatDetuneChannel(Chat::UserNLA const& nla)
{
	if(nla.radioIndex >= MAX_CLIENT_CHANNELS) return;

	ChatChannel& channel = ChatChannels[nla.radioIndex];
	channel.type = 0;
	channel.recv = false;
	channel.xmit = false;
	channel.name[0] = '\0';
}

/************************************************************************************************
	lua helper functions
*************************************************************************************************/

void uiChatSetRadio(UINT32 radioIndex, UINT32 type)
{
	if(radioIndex >= MAX_CLIENT_CHANNELS)
		return;

	if(ChatChannels[radioIndex].type == type)
		return;

	chatTuneRadioToChannel(PlayerClient()->ChatConnection(), (UCHAR)radioIndex, type);
}

UINT32 uiChatGetRadio(UINT32 radioIndex)
{
	return (radioIndex < MAX_CLIENT_CHANNELS) ? ChatChannels[radioIndex].type: NO_CHANNEL;
}

/************************************************************************************************
	uiChatGCIMessage
************************************************************************************************/

void uiChatGCIMessage(Chat::GCIMessage const& pgciMsg, bool alertOnly)
{
	int uiMsgNo = 0;
	int gameEvent = GAMEEVENT_INVALID;
	const char* specifics = nullptr;
	bool facilityEvent = false;

	switch(pgciMsg.gci)
	{
		case GCI_FACILITY_CAPTURED:
			gameEvent = GAMEEVENT_ALERT_FACILITY;
			facilityEvent = true;
			specifics = "captured";
			uiMsgNo = 125;
		break;
		case GCI_FACILITY_ACQUIRED:
			gameEvent = GAMEEVENT_ALERT_FACILITY;
			facilityEvent = true;
			specifics = "acquired";
			uiMsgNo = 126;
		break;
		case GCI_FACILITY_LIBERATED:
			gameEvent = GAMEEVENT_ALERT_FACILITY;
			facilityEvent = true;
			specifics = "liberated";
			uiMsgNo = 127;
		break;
		case GCI_FACILITY_DELIBERATED:
			gameEvent = GAMEEVENT_ALERT_FACILITY;
			facilityEvent = true;
			specifics = "deliberated";
			uiMsgNo = 128;
		break;
		case GCI_CP_CAPTURED:
			gameEvent = GAMEEVENT_ALERT_CP;
			specifics = "captured";
			uiMsgNo = 129;
		break;
		case GCI_CP_ACQUIRED:
			gameEvent = GAMEEVENT_ALERT_CP;
			specifics = "acquired";
			uiMsgNo = 130;
		break;
		case GCI_CP_LIBERATED:
			gameEvent = GAMEEVENT_ALERT_CP;
			specifics = "liberated";
			uiMsgNo = 131;
		break;
		case GCI_CP_DELIBERATED:
			gameEvent = GAMEEVENT_ALERT_CP;
			specifics = "deliberated";
			uiMsgNo = 132;
		break;
		case GCI_CP_UNDER_ATTACK:
			gameEvent = GAMEEVENT_ALERT_CPATTACKED;
			uiMsgNo = 245;
		break;
		case GCI_CP_ENEMY_GROUND_SPOTTED:
			gameEvent = GAMEEVENT_ALERT_ENEMY;
			specifics = "ground";
			uiMsgNo = 246;
		break;
		case GCI_CP_ENEMY_ARMOR_SPOTTED:
			gameEvent = GAMEEVENT_ALERT_ENEMY;
			specifics = "armor";
			uiMsgNo = 247;
		break;
		case GCI_CP_ENEMY_AIR_SPOTTED:
			gameEvent = GAMEEVENT_ALERT_ENEMY;
			specifics = "air";
			uiMsgNo = 248;
		break;
		case GCI_CP_ENEMY_SEA_SPOTTED:
			gameEvent = GAMEEVENT_ALERT_ENEMY;
			specifics = "sea";
			uiMsgNo = 249;
		break;
		case GCI_CP_OBJECTIVE_WITHDRAWN:
			gameEvent = GAMEEVENT_ALERT_OBJECTIVE;
			specifics = "withdrawn";
			uiMsgNo = 519;
		break;
        default:
            // no other valid game events
            break;
	};

	if(gameEvent)
	{
		// add in a game event here
		MissionDetail& mission = PlayerClient()->SelectedMission();

		if(mission.m_isCurrent)
		{
			const StratDB::StratObject* eventTarget = nullptr;
			StratDB::StratObject* missionStrat = StratDB::FindStratObject(mission.m_origin);

			if(missionStrat != nullptr && missionStrat->IsRelated(pgciMsg.where))
			{
				eventTarget = StratDB::FindStratObject((facilityEvent) ? pgciMsg.facility: pgciMsg.where);
			}
			else
			{
				missionStrat = StratDB::FindStratObject(mission.m_target);

				if(missionStrat != nullptr && missionStrat->IsRelated(pgciMsg.where))
					eventTarget = StratDB::FindStratObject((facilityEvent) ? pgciMsg.facility: pgciMsg.where);
			}

			if(eventTarget)
			{
				int argc = 1;
				jsval argv[3];

				argv[0] = INT_TO_JSVAL(gameEvent);

				if(specifics)
				{
					argv[1] = phui::js::NewString(specifics);
					argv[2] = phui::js::NewString(eventTarget->DisplayName());
					argc = 3;
				}
				else
				{
					argv[1] = phui::js::NewString(eventTarget->DisplayName());
					argc = 2;
				}

				uiGameEvent(argv, argc);
			}
		}

		if(pgciMsg.gci == GCI_CP_UNDER_ATTACK && pgciMsg.facility != nullObjID)
		{
			Chat::GCIMessage facCapMsg(pgciMsg);

			facCapMsg.gci = GCI_FACILITY_CAPTURED;
			uiChatGCIMessage(facCapMsg, true);
		}
	}

	if(!alertOnly)
		uiChatPrintLineMessageLoc(IDE_LANG_UI, LSTR_HQ, uiMsgNo, UTF16(pgciMsg.name), nullptr);
}

bool uiChatInitWordFilter()
{
	// reset the word filter
	_wordFilter.clear();

	std::string path = "languages/";
	path += g_szLanguages[gLanguage];
	path += "/badwords.xml";

	// create a loader
	BadWordLoader loader(path.c_str());

	// load the filter with rules
	loader.Load(&_wordFilter);

	return true;
}

/************************************************************************************************
 * const wchar_t* filterText - take source text, run it through a language filter and return the
 * result.
 ************************************************************************************************/

static const wchar_t* _filterText(const wchar_t* source)
{

	// If the user has disabled the language filter, then just return the input.
	if (_wordFilter.empty() || cfgPrefsGetBool("app.languagefilter", true) == false)
		return source;

	/////////// STATIC VARS

	// filtered wide character buffer
	static std::wstring filterBuffer;

	// matches that hit for the current test
	static RENS::wcmatch matches;

	// set the current filterBuffer to the incoming source
	filterBuffer = source;

	// filter iterator
	OBSCENITY_FILTER::const_iterator it;

	// It could be in the cuss pool, so compare.
	for (it = _wordFilter.begin(); it != _wordFilter.end(); ++it )
	{
		bool found = false;

		try
		{
			found = RENS::regex_search(filterBuffer.c_str(), matches, *it);
		}
		catch (RENS::regex_error& /* e */)
		{
			// not a valid regular expression
		}
		catch (std::exception& /* e */)
		{
			// not a valid regular expression
		}

		if(found)
		{
			for(unsigned int m = 0; m < matches.size(); ++m)
			{
				static OBSCENITY_RULE filter;
				static std::wstring match;

				// get the current match
				match = matches.str(m);

				// set the detail regex
				filter.assign(match, RENS::regex_constants::icase);

				for(unsigned int c = 0; c < match.size(); ++c)
					match[c] = L'#';

				try
				{
					// filter it
					filterBuffer =  RENS::regex_replace(filterBuffer, filter, match);
				}
				catch (RENS::regex_error& /* e */)
				{
					// not a valid regular expression
				}
				catch (std::exception& /* e */)
				{
					// not a valid regular expression
				}
			}
		}
	}

	return filterBuffer.c_str();
}

/*
static const wchar_t* _filterTextOld(const wchar_t* source)
{
	// If the user has disabled the language filter, then just return the input.
	if (_wordFilter.empty() || cfgPrefsGetBool("app.languagefilter", true) == false)
		return source;

	/////////// STATIC VARS

	// filtered wide character buffer
	static wchar_t filterBuffer[MAX_MESSAGE_LENGTH + 8];

	// size of replacements string
	static const UINT32 NUM_REPLACEMENTS = 8;

	// used to rotate characters used
	static UINT32 replacement = NUM_REPLACEMENTS;

	// replacements string
	static const wchar_t replacements[NUM_REPLACEMENTS + 1] = L"%*$&^!@#";

	/////////// TEMP VARS

	// filter iterator
	OBSCENITY_FILTER::const_iterator it;

	// pointer to the wide character filtered buffer
	wchar_t* filteredPtr = filterBuffer; // init dest to beginning of wide filtered buffer

	// used to find word breaks
	const wchar_t* wordEnd = nullptr;

	// point to the converted wide string source
	const wchar_t* sourcePtr = source;

	////////// FILTER SOURCE

	while ( *sourcePtr )
	{
		// Skip any kind of noise between words
		while ( isspace(*sourcePtr) )
			*(filteredPtr++) = *(sourcePtr++);

		// Allow for end of string
		if ( *sourcePtr == 0 )
			break;

		// Lets find the first non-alphabetical crap
		wordEnd = sourcePtr;

		// move forward until the end of the string or the next space
		while (*wordEnd && !isspace(*wordEnd) )
			++wordEnd;

		// Find the size of the "word"/token, and see if it matches the cuss pool
		UINT32  size = wordEnd - sourcePtr;
		if ( size >= _minFilterWord )
		{
			// It could be in the cuss pool, so compare.
			for ( it = _wordFilter.begin(); it != _wordFilter.end(); ++it )
			{
				const OBSCENITY_RULE& rule = *it;
				const UINT32 ruleSize = rule.text.size();

				// nothing to do if the rule is empty
				if(!ruleSize) continue;

				// Check sizes first before doing string compares
				if ( size == ruleSize || (rule.isPrefix && size >= ruleSize) )
				{
					// Does as much of the cuss as we have match?
					if ( wstrncasecmp(sourcePtr, rule.text.c_str(), ruleSize) == 0 )
					{
						// Found a cuss, replace it with characters from "replacements".
						for ( UINT32 i = 0; i < ruleSize; ++i )
							*(filteredPtr++) = replacements[replacement++ % NUM_REPLACEMENTS];

						// Now check here for the start of another cuss-word
						sourcePtr += ruleSize;
						break;
					}
				}
			}
		}

		// In any other case, source will be short of wordEnd, so copy the remaining characters.
		while ( sourcePtr < wordEnd )
			*(filteredPtr++) = *(sourcePtr++);
	}

	// terminate the string
	*filteredPtr = 0;

	// return the filtered string
	return filterBuffer;
}
*/

/************************************************************************************************
	uiChatHostMessage
 ************************************************************************************************/

void uiChatHostMessage(Chat::HostMessage const& message)
{
	int icons[CHATICON_MAX] = {-1,-1,-1};
	int j = 0;
	for(int i=0; i < MAX_CLIENT_CHANNELS && j < 2; i++)
	{
		if(message.radioType[i])
			icons[j++] = message.radioType[i]-1;
	}

	// The icons on the host are 1 indexed.
	icons[2] = message.icon - 1;

	// Eliminate obscenities
	const wchar_t* msg = _filterText(uiUTF8toUTF16(message.msg));

	if (message.handle > 0)
	{
		UINT32 chatColor = MSG_COLOR; // default

		const wchar_t* callsign = chatGetWideCallsign(message.handle);

		// Determine chat message color
		// If we add more chat color selections, maybe move it to a function?
		// my currently selected mission (holds leader info)
		GameClient* c = PlayerClient();

		MissionDetail& mission	= c->SelectedMission();
		BrigadeControlList* oic	= c->LookupBrigadeOIC(c->BrigadeSelected().m_detail.brigadeID());

		if((mission.m_isCurrent && mission.m_poster == message.handle) ||
		   (oic && oic->m_officer == message.handle))
		{
			chatColor = gIconMissionLeaderColor | colorALPHA;
		}

		// print the chat message
		uiChatPrintLineIcon(icons, chatColor, L"%ls: %ls", callsign, msg);

		if(	message.icon == MAGIC_DOT_R_COUNTRY && *callsign &&
			message.handle != c->Personas().m_callsign)
		{
			lastplayerhandle = callsign;
			char script[80];
			sprintf(script, "game.onprivatemessage('%s', %u)", chatGetCallsign(message.handle), teulTime());
			c->jsclient->RunScript(script);
		}
	}else
	{
		uiChatPrintLineIcon(icons, MSG_COLOR, L"%ls", msg);
	}
}

/************************************************************************************************
	uiChatWindow
 ************************************************************************************************/

phui::View* uiChatWindow()
{
	return _chatwin;
}

/************************************************************************************************
	uiChatIconCreate
*************************************************************************************************/

phui::View* uiChatIconCreate(_xmlNode* node, phui::View* parent)
{
	if(!_commicons)
		_commicons = drawIconLoad("ui/uidata/chat/commicons3");

	return new DrawIconView(_commicons, -1);
}

/************************************************************************************************
	uiChatInputCreate
*************************************************************************************************/

phui::View* uiChatInputCreate(_xmlNode* node, phui::View* parent)
{
	ChatInput* input = new ChatInput;

	input->SetAutoCompleteCallback(uiDotCommandMatch);

	return input;
}

/************************************************************************************************
	jsChatIcon
*************************************************************************************************/

class jsChatIcon : public phui::DOM::ElementDOM
{
public:
	jsChatIcon(DrawIconView* dv, phui::DOM::ElementDOM*);

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

	JS_CLASS_DATA
	enum{ CHANNELNAME_PROP=100 };

	DrawIconView* iconview;

protected:
	virtual ~jsChatIcon();
};

JSClass jsChatIcon::jsclass = JS_CLASS_MACRO("chaticon");
JSFunctionSpec jsChatIcon::jsfunctions[] =
{
	{0}
};
JSPropertySpec jsChatIcon::jsproperties[] =
{
	{"channelname",		jsChatIcon::CHANNELNAME_PROP,   	JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	uiMapCreateDOM
*************************************************************************************************/

phui::DOM::ElementDOM* uiChatIconCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent)
{
	DrawIconView* dv = dynamic_cast<DrawIconView*>(view);
	if(dv)
		return  new jsChatIcon(dv, parent);

	return 0;
}

/************************************************************************************************
	jsChatIcon
*************************************************************************************************/

jsChatIcon::jsChatIcon(DrawIconView* v, phui::DOM::ElementDOM* d) : ElementDOM(v,d, &jsChatIcon::jsclass)
{
	iconview = v;
	DefineFunctions(jsfunctions);
	DefineProperties(jsproperties);
}

jsChatIcon::~jsChatIcon()
{
}

JSBool jsChatIcon::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case CHANNELNAME_PROP:
			*val = JSVAL_NULL;
		break;
		default:
			phui::DOM::ElementDOM::GetProperty(ctx, propid, val);
		break;
	}
	return JS_TRUE;
}

JSBool jsChatIcon::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case CHANNELNAME_PROP:
		{
			const char* name = JS_GetStringBytes(JS_ValueToString(_jsctx, *val));
			if(!name || !strcasecmp(name, ""))
				iconview->icon = IDE_ICON_EMPTY_ICON;
			else if(!strcasecmp(name, "Brigade"))
				iconview->icon = IDE_ICON_BRIGADE;
			else if(!strcasecmp(name, "Mission"))
				iconview->icon = IDE_ICON_MISSION;
			else if(!strcasecmp(name, "Squad"))
				iconview->icon = IDE_ICON_SQUAD;
			else if(!strcasecmp(name, "Origin"))
				iconview->icon = IDE_ICON_ORIGIN;
			else if(!strcasecmp(name, "Objective"))
				iconview->icon = IDE_ICON_OBJECTIVE;
			else if(!strcasecmp(name, "Side"))
				iconview->icon = IDE_ICON_WORLD;
			else if(!strcasecmp(name, "Help"))
				iconview->icon = IDE_ICON_HELP;
			else if(!strcasecmp(name, "Say"))
				iconview->icon = IDE_ICON_SAY;
			else if(!strcasecmp(name, "Ops"))
				iconview->icon = IDE_ICON_THEATER;
			else
				iconview->icon = IDE_ICON_PRIVATE;
		}
		break;
		default:
			phui::DOM::ElementDOM::SetProperty(ctx, propid, val);
		break;
	}
	return JS_TRUE;
}

/************************************************************************************************
	KeyDown
*************************************************************************************************/

bool ChatInput::KeyDown(int keycode, unsigned short unicode)
{
	TextControl::KeyDown(keycode, unicode);
	static const int BACKSPACE = 8;

	std::wstring text = WideText();

	if( (text == L".r " || text == L"/r ") && (keycode == BACKSPACE) )
	{
		SetValue(""); // Clear the chat bar if we use backspace while we have ".r " in the chat bar.
	}

	else if((text == L".r " || text == L"/r ") && (keycode != BACKSPACE))
	{
		text += lastplayerhandle.c_str();
		text += L" ";
		SetValue(text.c_str());
	}
    else if (text.substr(0,5) == L".bug " || text.substr(0,5) == L"/bug " || text.substr(0,8) == L".report " || text.substr(0,8) == L"/report " )
    {
        if (text.length() > MAX_REPORT_LEN)
            SetValue(text.substr(0, MAX_REPORT_LEN).c_str());
    }
	else
	{
		/*
		switch(keycode)
		{
			case phui::KEYCODE_UP:
				break;
			case phui::KEYCODE_DOWN:
				break;
		}
		*/
	}

	return true;
}

/************************************************************************************************
	uiChatMessagePlayer
*************************************************************************************************/

void uiChatMessagePlayer(const wchar_t* handle)
{
	if(_chatmsg)
	{
		uiChatInput(IAC_ENTER);
		std::wstring msg = L".m ";
		msg += handle;
		msg += L" ";
		_chatmsg->SetValue(msg.c_str());
	}
}

#include "rn/rnmain.h"

LOGICAL rnmainListLocText(SINT32 category, SINT32 locID)
{
	// lookup the loc key
	const char* lockey = (char*)loclLangGetString((ENUM_CATEGORIES)category, (UINT32)locID);

	// send it to chat
	rnmainListText(uiLocString(lockey).c_str());

	return TRUE;
}

LOGICAL rnmainListLocColorText(SINT32 category, SINT32 locID, UINT32 uColor)
{
	// lookup the loc key
	const char* lockey = (char*)loclLangGetString((ENUM_CATEGORIES)category, (UINT32)locID);

	// send it to chat
	rnmainListColorText(uColor, uiLocString(lockey).c_str());

	return TRUE;
}

LOGICAL rnmainListEmoteText(SINT32 category, SINT32 locID, UINT32 uColor, const WCHAR* prepend, const WCHAR* append)
{
	const char* lockey = (char*)loclLangGetString((ENUM_CATEGORIES)category, (UINT32)locID);
	const std::wstring& message = uiLocString(lockey);

	if(prepend)
	{
		if(append)
		{
			rnmainListColorText(uColor, L"%ls%ls%ls", prepend, message.c_str(), append);
		}
		else
		{
			rnmainListColorText(uColor, L"%ls%ls", prepend, message.c_str());
		}
	}
	else if(append)
	{
		rnmainListColorText(uColor, L"%ls%ls", message.c_str(), append);
	}
	else
	{
		rnmainListColorText(uColor, message.c_str());
	}

	return TRUE;
}


LOGICAL rnmainListText(const char* text, ...)
{
	if ( nullptr == text )
		return FALSE;

 	va_list marker;
	char buf[256];

	va_start(marker, text);
	vsnprintf(buf, 255, text, marker);

	uiChatPrintLine(uiUTF8toUTF16(buf));
	va_end(marker);

	return(TRUE);
}

LOGICAL rnmainListText(const wchar_t* text, ...)
{
	if ( nullptr == text )
		return FALSE;

 	va_list marker;
	wchar_t buf[256];

	va_start(marker, text);
	vswprintf(buf, 255, text, marker);

	uiChatPrintLine(buf);
	va_end(marker);

	return(TRUE);
}

LOGICAL rnmainListColorText(UINT32 uColor, const WCHAR* text, ...)
{
 	va_list marker;
	wchar_t buf[256];

	va_start(marker, text);
	vswprintf(buf, 255, text, marker);

	uiChatPrintLineColor(uColor, buf);

	va_end(marker);

	return(TRUE);
}


