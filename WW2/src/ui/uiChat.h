#pragma once

#include "phui/View.h"
#include "phui/DOM.h"

struct _xmlNode;

#include "chat/chatPrototypes.h"		// Common chat structure protos

enum
{
	CHATICON_COUNTRY
,	CHATICON_RX1
,	CHATICON_RX2
,	CHATICON_MAX
};

#define CHANNEL_NAME_LEN 50

/************************************************************************************************
	uiChat
*************************************************************************************************/

extern phui::View* uiChatCreate(_xmlNode*, phui::View* parent);
extern phui::View* uiChatIconCreate(_xmlNode*, phui::View* parent);
extern phui::View* uiChatInputCreate(_xmlNode*, phui::View* parent);
extern phui::DOM::ElementDOM* uiChatIconCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent);

extern int uiChatPrintLine(const char* format, ...);
extern int uiChatPrintLine(const wchar_t* format, ...);
extern void uiChatPrintUILocLine(int icon[CHATICON_MAX], unsigned int, const char* key, ...); // prints a localized line from ui.dtd
extern void uiChatPrintLineColor(unsigned int, const wchar_t* format, ...);
extern void uiChatPrintLineIcon(int icon[CHATICON_MAX], unsigned int color, const wchar_t* format, ...);
extern void uiChatPrintLineFeedback(int locgroup, int locid, bool iserror, ...);

extern void uiChatInvoke();

extern bool uiChatOK();
extern void uiChatShow();
extern void uiChatHide(bool close=false);
extern void uiChatSyncToCycleSetting();
extern unsigned int uiChatToggleHeight(int param, unsigned int functionNum, unsigned int alwaysZero);

extern bool uiChatInput(unsigned int iac);

extern void uiChatTuneToChannel(Chat::TuneToChannel const&);
extern void uiChatDetuneChannel(Chat::UserNLA const&);

extern void uiChatSetRadio(UINT32 radioIndex, UINT32 type);
extern UINT32 uiChatGetRadio(UINT32 radioIndex);

extern void uiChatPrintLineMessageLoc(int loc, int prefix, int message
										, const wchar_t* const arg1=L"", const wchar_t* const arg2=L""
										, const wchar_t* const arg3=L"", const wchar_t* const arg4=L""
										, const wchar_t* const arg5=L"", const wchar_t* const arg6=L""
										);
extern void uiChatGCIMessage(Chat::GCIMessage const&, bool alertOnly = false);
extern void uiChatHostMessage(Chat::HostMessage const&);

extern void uiChatMessagePlayer(const wchar_t* handle);

extern bool uiChatInitWordFilter();
extern void uiChatInit();
extern void uiChatKill();

extern phui::View* uiChatWindow();

struct ChatChannel
{
	ChatChannel() : name(L""), xmit(false), recv(false), type(0) {}
	std::wstring name;
	bool xmit, recv;
	int type;
};

extern ChatChannel ChatChannels[6];

enum
{
	IDE_ICON_ENGLAND,			// 0
	IDE_ICON_USA,
	IDE_ICON_FRANCE,
	IDE_ICON_GERMANY,
	IDE_ICON_ITALY,
	IDE_ICON_JAPAN,
	IDE_ICON_COMMONWEALTH,
	IDE_ICON_CHINA,
	IDE_ICON_RUSSIA,
	IDE_ICON_WORLD,
	IDE_ICON_THEATER,			// 10
	IDE_ICON_MISSION,
	IDE_ICON_SQUAD,
	IDE_ICON_ORIGIN,
	IDE_ICON_PRIVATE,
	IDE_ICON_ALLIED,
	IDE_ICON_AXIS,
	IDE_ICON_GCI,
	IDE_ICON_BRIGADE,
	IDE_ICON_HELP,
	IDE_ICON_OBJECTIVE,			// 20
	IDE_ICON_HC,
	IDE_ICON_SAY,
	IDE_ICON_NOTE,
	IDE_ICON_ENEMY_SAY,
	IDE_ICON_NOT_USED9,
	IDE_ICON_NOT_USED10,
	IDE_ICON_NOT_USED11,
	IDE_ICON_CAUTION,
	IDE_ICON_WHISPER,
	IDE_ICON_YELL,				// 30
	IDE_ICON_NOT_USED15,
	IDE_ICON_FLAG_ENGLAND,
	IDE_ICON_FLAG_USA,
	IDE_ICON_FLAG_FRANCE,
	IDE_ICON_FLAG_GERMANY,
	IDE_ICON_FLAG_ITALY,
	IDE_ICON_FLAG_JAPAN,
	IDE_ICON_FLAG_COMMONWEALTH,
	IDE_ICON_FLAG_CHINA,
	IDE_ICON_FLAG_RUSSIA,		// 40
	IDE_ICON_FLAG_NOT_USED1,
	IDE_ICON_FLAG_NOT_USED2,
	IDE_ICON_FLAG_NOT_USED3,
	IDE_ICON_FLAG_NOT_USED4,
	IDE_ICON_FLAG_NOT_USED5,
	IDE_ICON_FLAG_NOT_USED6,
	IDE_ICON_FLAG_NOT_USED7,
	IDE_ICON_EMPTY_ICON,
	NUMBER_OF_CHAT_ICONS
};
