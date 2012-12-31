#pragma once

namespace phui
{

enum 
{
	ANY_EVENT   = 'ANYX',
	KEY_DOWN	= 'KEYD',
	KEY_UP		= 'KEYU',
	MOUSE_DOWN  = 'MSDN',
	MOUSE_UP	= 'MSUP',
	MOUSE_DRAG  = 'MSDR',
	MOUSE_SCROLL= 'MSCR',
	MOUSE_MOVE  = 'MSMV',
	CLOSE_MSG   = 'CLOS',
	QUIT_MSG	= 'QUIT',
	OPEN_MSG	= 'OPEN',
	SHOW_MSG	= 'SHOW',
	HIDE_MSG	= 'HIDE',
	SELECT_MSG	= 'SLCT',
	DBLSELECT_MSG = 'DSLT',
	TAB_MSG		= 'TABM'
};

enum 
{
	MOUSE_BUTTON_1 = 0x1,
	MOUSE_BUTTON_2 = 0x2,
	MOUSE_BUTTON_3 = 0x4
};

enum 
{
	MOUSE_ENTER,
	MOUSE_OVER,
	MOUSE_EXIT
};

// Message fields
#define UI_WHEN					"ui/when"		// long milliseconds

// Key events
#define UI_KEYCODE				"ui/keycode"	//  long key code
#define UI_KEY_UNICODE			"ui/unicode"	//  long unicode value

// Mouse events
#define UI_MOUSE_X				"ui/x"			//  float position screen co-ordinates
#define UI_MOUSE_Y				"ui/y"				
#define UI_MOUSE_BUTTON_MASK	"ui/buttonmask"	//  long mask state of mouse buttons
 
// Mouse up, down
#define UI_MOUSE_BUTTON			"ui/button"		//  long which button generated this event

// Mouse move
#define UI_MOUSE_MOVESTATE		"ui/movestate"  //  long Move State

// Mouse scroll
#define UI_MOUSE_SCROLLDIR		"ui/scrolldir"  //  long (1,-1)

// Mouse drag
#define UI_MOUSE_DRAG_START		"ui/dragstart"  // bool

// Control message
#define UI_CONTROL_SOURCE		"ui/source"		// Pointer to Control
#define UI_CONTROL_VALUE		"ui/value"		// Scalar value

// Close message
#define UI_VIEW_PTR				"ui/view"		// Pointer to view to be closed in CLOSE_MSG

// Open open message
#define UI_WINDOW_NAME			"ui/windowname" // Filename of window to open in OPEN_MSG

}
