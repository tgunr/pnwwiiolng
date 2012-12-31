/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991-1994  Riley Rainey
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 dated June, 1991.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program;  if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave., Cambridge, MA 02139, USA.
 */
#include <string.h>
#include <stdio.h>
#include <pwd.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/MessageB.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/CascadeB.h>
#include <Xm/BulletinB.h>
#include <Xm/DrawingA.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/List.h>
#include <Xm/Label.h>

#define ALLOCATE_SPACE
#include <client.h>

XtInputId	input_id;
int		ts;
SVCXPRT		*xprt;

void	fatal_error();
void	set_window_title();

void	save_widget_id();
void	manage_by_id();
void 	unmanage_by_id();
void	exit_game();
void	PostMessage();
void	new_game(), game_selected(), aircraft_selected();
void	record_game_selection();
void	CvtStringToStringList();

#define MAX_TITLE_LEN	128
char	wintitle[MAX_TITLE_LEN];
Widget	toplevel;

static XmString	gameName[MAXGAMES];
int	selectedGame = -1;

static XmString	ateam[MAXPLAYERS+1], bteam[MAXPLAYERS+1];
static int	acount, bcount;
static XmString	aircraftStrings[MAXAIRCRAFT];
static int	aircraftCount;


static XmString	cerrorMessage = NULL;
static XmString	lastMessage = NULL;

extern CLIENT *clnt;

extern	int InitACMClient(), SendGameQuery();
extern	void ProcGSInput(), InvalidateGameEntries();
extern	int EnterGame();
extern	void ResizeRenderArea();
extern	void ExposeRenderArea();
extern	void UpdateClientDatabase();
extern	void ImportPlayerUpdate PARAMS((acm_player_update *, craft *c));

static char * game_over =
"Well, the server didn't say how it happened,\n\
but you are dead.  You can re-enter the game from \n\
the \"File\" menu.";

/*
 *  fallback resources
 */

static String	fallback[] = {
	"*radarForeground:		#59fe00",
	"*radarBackground:		#094200",
	"*skyColor:			#93bde4",
	"*Background:			GhostWhite",
	"*fontList:			*helvetica-medium-r-normal--14*",
	"*menu_bar*Foreground:		GhostWhite",
	"*menu_bar*Background:		#454fc6",
	"*play.labelString:		Play a Game...",
	"*quit.labelString:		Quit",
	"*quit.acceleratorText:		Alt+Q",
	"*quit.accelerator:		Meta<Key>q:",
	"*error_dialog.dialogTitle:	Error Message",
	"*error_dialog.okLabelString:	Continue",
	"*message_dialog.dialogTitle:	Message",
	"*message_dialog.okLabelString:	Continue",
	"*XmPushButton.shadowThickness:	3",
	"*XmForm.verticalSpacing:	10",
	"*XmForm.horizontalSpacing:	10",
	"*render_form.verticalSpacing:	0",
	"*render_form.horizontalSpacing:0",
	"*render_form.width:		1200",
	"*render_form.height:		750",
	"*render_area.background:	black",
	"*blue_team_list.visibleItemCount:  5",
	"*red_team_list.visibleItemCount:   5",
	"*aircraft_list.visibleItemCount:   4",	
	"*team_display.allowOverlap:	false",
	"*game_selection.dialogTitle:	Select a Game",
	"*game_selection.labelString:	Select a Game:",
	"*game_selection.applyLabelString: Start a New Game",
	"*aircraft_dialog.dialogTitle:	Choose Your Aircraft",
	"*red_label.labelString:	Red Team:",
	"*blue_label.labelString:	Blue Team:",
	"*prompt.labelString: Select the aircraft you wish to fly:",
	NULL
	};

static XrmOptionDescRec options[] = {
	{"-skycolor",	"*skyColor", 	XrmoptionSepArg, NULL },
	{"-callsign",	"*callsign", 	XrmoptionSepArg, NULL },
	{"-servers",	"*servers", 	XrmoptionSepArg, NULL },
	{"-s",		"*servers", 	XrmoptionSepArg, NULL },
	};

#define XtNskyColor	"skyColor"
#define XtCSkyColor	"SkyColor"
#define XtNcallsign	"callsign"
#define XtCCallsign	"Callsign"
#define XtNservers	"servers"
#define XtCServers	"Servers"

static XtResource resources[] = {
	{
		XtNskyColor,
		XtCSkyColor,
		XmRString,
		sizeof(String),
		XtOffset(AppDataPtr, skycolor),
		XmRString,
		(caddr_t) ""
	},
	{
		XtNcallsign,
		XtCCallsign,
		XmRString,
		sizeof(String),
		XtOffset(AppDataPtr, callsign),
		XmRImmediate,
		(caddr_t) NULL
	},
	{
		XtNservers,
		XtCServers,
		"StringList",
		sizeof(String *),
		XtOffset(AppDataPtr, servers),
		XmRString,
		(caddr_t) ""
	},
	{
		"radarForeground",
		"RadarForeground",
		XmRString,
		sizeof(String),
		XtOffset(AppDataPtr, radar_foreground),
		XmRString,
		(caddr_t) ""
	},
	{
		"radarBackground",
		"RadarBackground",
		XmRString,
		sizeof(String),
		XtOffset(AppDataPtr, radar_background),
		XmRString,
		(caddr_t) ""
	},
	{
		"appFont",
		"AppFont",
		XmRFontStruct,
		sizeof(XFontStruct *),
		XtOffset(AppDataPtr, font),
		XmRString,
		(caddr_t) "*-clean-medium-r-*-*-12-*"
	}
	};

#define MAX_ARGS	16
#define APP_CLASS	"Acm"

Widget		CreateHelp();
XtWorkProcId	work_id;
Boolean		PerformUpdate();

/*
 *  Action declarations
 */

extern void	SetThrottle(), Afterburner();
extern void	ToggleGear(), ToggleBrakes(), SetRudder(), SetView();
extern void	Trigger(), SelectWeapon(), LaunchDrone();

static XtActionsRec actions[] = {
	{"set-throttle",	SetThrottle},
	{"afterburner",		Afterburner},
	{"toggle-gear",		ToggleGear},
	{"toggle-brakes",	ToggleBrakes},
	{"set-rudder",		SetRudder},
	{"look",		SetView},
	{"trigger",		Trigger},
	{"select-weapon",	SelectWeapon},
	{"launch-drone",	LaunchDrone},
	};

static char trans[] =
	"<Key>1:	set-throttle(0) afterburner(0)	\n\
	 <Key>2:	set-throttle(-512)	\n\
	 <Key>3:	set-throttle(+512)	\n\
	 <Key>4:	set-throttle(32768)	\n\
	 <Key>6:	set-throttle(32768) afterburner(1) \n\
	 <Key>a:	afterburner()		\n\
	 <Key>b:	toggle-brakes()		\n\
	 <Btn2Down>:	trigger(press)		\n\
	 <Btn2Up>:	trigger(release)	\n\
	 <Btn3Down>:	select-weapon()		\n\
	 <Key>g:	toggle-gear()		\n\
	 <Key>z:	set-rudder(+1024)	\n\
	 <Key>x:	set-rudder(0)		\n\
	 <Key>c:	set-rudder(-1024)	\n\
	 <Key>l:	launch-drone()		\n\
	 <Key>osfPageUp: look(up)	\n\
	 <Key>osfUp:	look(forward)	\n\
	 <Key>osfDown:	look(aft)	\n\
	 <Key>osfLeft:	look(left)	\n\
	 <Key>osfRight:	look(right)	\n\
	 <Key>KP_8:	look(forward)	\n\
	 <Key>KP_5:	look(up)	\n\
	 <Key>KP_2:	look(aft)	\n\
	 <Key>KP_6:	look(right)	\n\
	 <Key>KP_4:	look(left)";
			
XtAppContext	context;

extern struct passwd *getpwent();

void
main (argc, argv)
int	argc;
char	**argv;
{

	Widget		main_window;
	Widget		menu_bar;
	Widget		menu_pane;
	Widget		button;
	Widget		cascade;
	Widget		frame;
	Widget		form;
	Widget		aform;
	Widget		help_dialog;
	Widget		w;
	Arg		args[MAX_ARGS];
	int		n;
	char		*charptr;
	XtTranslations	translations;
	struct passwd	*pw;

	player_id = -1;

	if ((pw = getpwuid(geteuid())) == (struct passwd *) NULL) {
		fprintf (stderr, "Yow. I can't get your username\n");
		strcpy (handle, "* unknown *");
	}
	else {
		strncpy (handle, pw->pw_name, sizeof(handle));
	}

	toplevel = XtAppInitialize (&context, APP_CLASS, options,
		XtNumber(options), &argc, argv, fallback, NULL, 0);

	XtAppAddConverter (context,
		XmRString,
		"StringList",
		CvtStringToStringList,
		(XtConvertArgList) NULL, (Cardinal) 0); 

	XtGetApplicationResources (toplevel, &app_data, resources,
		XtNumber(resources), NULL, 0);
	
	work_id = XtAppAddWorkProc (context, PerformUpdate, (XtPointer) NULL);

	skycolor = app_data.skycolor;

	XtAppAddActions (context, actions, XtNumber(actions));

	n = 0; 
	main_window = XmCreateMainWindow (toplevel, "main", args, n);
	XtManageChild (main_window);

	n = 0;
	menu_bar = XmCreateMenuBar (main_window, "menu_bar", args, n); 
	XtManageChild (menu_bar);

/*
 *  The File menu
 */

	n = 0;
	menu_pane = XmCreatePulldownMenu (menu_bar, "file_pulldown", args, n);

	n = 0;
	button = XmCreatePushButton (menu_pane, "play", args, n);
	XtManageChild (button);
	XtAddCallback (button, XmNactivateCallback, manage_by_id,
		game_selection_id);

	n = 0;
	button = XmCreatePushButton (menu_pane, "quit", args, n);
	XtManageChild (button);
	XtAddCallback (button, XmNactivateCallback, exit_game, NULL);

	n = 0;
	XtSetArg (args[n], XmNsubMenuId, menu_pane);  n++;
	cascade = XmCreateCascadeButton (menu_bar, "File", args, n);
	XtManageChild (cascade);

/*
 *  The help button
 */

	widgets[help_id] = CreateHelp(toplevel);

	n = 0;
	cascade = XmCreateCascadeButton (menu_bar, "Help", args, n);
	XtManageChild (cascade);
	XtAddCallback (cascade, XmNactivateCallback, manage_by_id,
		(XtPointer) help_id);

	n = 0;
	XtSetArg (args[n], XmNmenuHelpWidget, cascade);  n++;
	XtSetValues (menu_bar, args, n);

/*
 *  The frame and rendering area
 */

	n = 0;
	XtSetArg (args[n], XmNmarginWidth, 2);  n++;
	XtSetArg (args[n], XmNmarginHeight, 2);  n++;
	XtSetArg (args[n], XmNshadowThickness, 1);  n++;
	XtSetArg (args[n], XmNshadowType, XmSHADOW_OUT);  n++;
	frame = XmCreateFrame (main_window, "render_frame", args, n);
	XtManageChild (frame);

	n = 0;
	form = XmCreateForm (frame, "render_form", args, n);
	XtManageChild (form);

	n = 0;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);  n++;
	widgets[render_id] = XmCreateDrawingArea (form, "render_area", args, n);
	XtManageChild (widgets[render_id]);

	XtAddCallback (widgets[render_id], XmNresizeCallback,
		ResizeRenderArea, (XtPointer) NULL);
	XtAddCallback (widgets[render_id], XmNexposeCallback,
		ExposeRenderArea, (XtPointer) NULL);

	translations = XtParseTranslationTable (trans);
	XtOverrideTranslations (widgets[render_id], translations);

/*
 *  Create a selection box dialog for choosing a game
 */

	n = 0;
	widgets[game_selection_id] = XmCreateSelectionDialog (toplevel,
		"game_selection", args, n);
	XtAddCallback (widgets[game_selection_id], XmNokCallback,
		game_selected, (XtPointer) NULL);
	XtAddCallback (widgets[game_selection_id], XmNcancelCallback,
		unmanage_by_id, (XtPointer) game_selection_id);
	XtAddCallback (widgets[game_selection_id], XmNapplyCallback,
		new_game, (XtPointer) NULL);

	w = XmSelectionBoxGetChild (widgets[game_selection_id], XmDIALOG_TEXT);
	XtUnmanageChild (w);
	w = XmSelectionBoxGetChild (widgets[game_selection_id], 
		XmDIALOG_SELECTION_LABEL);
	XtUnmanageChild (w);

	widgets[game_list_id] =
	    XmSelectionBoxGetChild(widgets[game_selection_id], XmDIALOG_LIST);

	n = 0;
	XtSetArg (args[n], XmNselectionPolicy, XmSINGLE_SELECT); n ++;
	XtSetValues (widgets[game_list_id], args, n);

	XtAddCallback (widgets[game_list_id], XmNsingleSelectionCallback,
		record_game_selection, NULL);

/*
 *  Create a general error message dialog
 */

	n = 0;
	widgets[error_id] = XmCreateErrorDialog (toplevel, "error_dialog",
		args, n);
	XtAddCallback (widgets[error_id], XmNokCallback,
		unmanage_by_id, (XtPointer) error_id);

	w = XmMessageBoxGetChild(widgets[error_id], XmDIALOG_CANCEL_BUTTON);
	XtUnmanageChild (w);

	w = XmMessageBoxGetChild(widgets[error_id], XmDIALOG_HELP_BUTTON);
	XtUnmanageChild (w);

/*
 *  Create a general message dialog
 */

	n = 0;
	widgets[message_id] = XmCreateMessageDialog (toplevel, "message_dialog",
		args, n);
	XtAddCallback (widgets[message_id], XmNokCallback,
		unmanage_by_id, (XtPointer) message_id);

	w = XmMessageBoxGetChild(widgets[message_id], XmDIALOG_CANCEL_BUTTON);
	XtUnmanageChild (w);

	w = XmMessageBoxGetChild(widgets[message_id], XmDIALOG_HELP_BUTTON);
	XtUnmanageChild (w);

/*
 *  Create the aircraft selection dialog
 */

	n = 0;
	widgets[aircraft_dialog_id] =
		XmCreateFormDialog (toplevel, "aircraft_dialog", args, n);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n ++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n ++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n ++;
	aform = XmCreateForm (widgets[aircraft_dialog_id], "team_display",
		args, n);
	XtManageChild (aform);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n ++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n ++;
	w = XmCreateLabel (aform, "blue_label", args, n);
	XtManageChild (w);

	n = 0;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n ++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n ++;
	XtSetArg (args[n], XmNtopWidget, w); n ++;
	widgets[blue_team_id] =
		XmCreateScrolledList(aform, "blue_team_list", args, n);
	XtManageChild (widgets[blue_team_id]);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n ++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n ++;
	w = XmCreateLabel (aform, "red_label", args, n);
	XtManageChild (w);

	n = 0;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n ++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n ++;
	XtSetArg (args[n], XmNtopWidget, w); n ++;
	widgets[red_team_id] =
		XmCreateScrolledList(aform, "red_team_list", args, n);
	XtManageChild (widgets[red_team_id]);

	n = 0;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n ++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n ++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n ++;
	XtSetArg (args[n], XmNtopWidget, aform); n ++;
	XtSetArg (args[n], XmNtopOffset, 25); n ++;
	w = XmCreateSeparator (widgets[aircraft_dialog_id], "separator",
		args, n);
	XtManageChild (w);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n ++;
	XtSetArg (args[n], XmNtopWidget, w); n ++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n ++;
	w = XmCreateLabel (widgets[aircraft_dialog_id], "prompt", args, n);
	XtManageChild (w);

	n = 0;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n ++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n ++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n ++;
	XtSetArg (args[n], XmNtopWidget, w); n ++;
	widgets[aircraft_list_id] =
		XmCreateScrolledList(widgets[aircraft_dialog_id],
		"aircraft_list", args, n);
	XtManageChild (widgets[aircraft_list_id]);

	n = 0;
	XtSetArg (args[n], XmNlabelString,
		XmStringCreateSimple ("  OK  ")); n ++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n ++;
	XtSetArg (args[n], XmNtopWidget, widgets[aircraft_list_id]); n ++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n ++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n ++;
	w = XmCreatePushButton(widgets[aircraft_dialog_id],
		"okay_button", args, n);
	XtAddCallback (w, XmNactivateCallback, aircraft_selected,
		(XtPointer) NULL);
	XtManageChild (w);

	XtRealizeWidget (toplevel);

/*
 *  Add the game socket to the list of fd's that Xt will manage.
 */

	InitACMClient ();
	InitRendering();

	InvalidateGameEntries();

/*
 *  Now shout out a request for game information ...
 */

	SendGameQuery ();

	XtAppMainLoop (context);

}

/* ARGSUSED */
void
manage_by_id (w, client_data, call_data)
Widget	w;
caddr_t	client_data;
caddr_t	call_data;
{
	int	i;

	i = (int) client_data;

	XtManageChild (widgets[i]);
}

/* ARGSUSED */
void
unmanage_by_id (w, client_data, call_data)
Widget	w;
caddr_t	client_data;
caddr_t	call_data;
{
	int	i;

	i = (int) client_data;

	XtUnmanageChild (widgets[i]);
}

/* ARGSUSED */
void
exit_game (w, client_data, call_data)
Widget	w;
caddr_t	client_data;
caddr_t	call_data;
{
	printf ("%d of %d updates were processed into frames.\n",
		updates_processed, update_count);
	exit (0);
}

/* ARGSUSED */
void
game_selected (w, client_data, call_data)
Widget	w;
caddr_t	client_data;
caddr_t	call_data;
{
	XtManageChild (widgets[aircraft_dialog_id]);
}

/* ARGSUSED */
void
aircraft_selected (w, client_data, call_data)
Widget	w;
caddr_t	client_data;
caddr_t	call_data;
{
	int	i, *list, count, aircraft_id = 0;

	XmListGetSelectedPos (widgets[aircraft_list_id], &list, &count);
	if (count == 1) {
		aircraft_id = list[0] - 1;
	}
	XtFree ((char *) list);
	if (selectedGame != -1) {
		if (EnterGame (selectedGame, aircraft_id) == -1) {
			PostMessage ("I was unable to enter the game.");
		}
	}

}

/* ARGSUSED */
void
new_game (w, client_data, call_data)
Widget	w;
caddr_t	client_data;
caddr_t	call_data;
{

}

/* ARGSUSED */
void
record_game_selection (w, client_data, call_data)
Widget	w;
caddr_t	client_data;
caddr_t	call_data;
{

	XmListCallbackStruct *p;
	GameInfo	*g;
	register int	i, n;
	Arg		args[4];
	acm_player_info *q;
	char		s[256];
	Dimension	red_width, blue_width;

/*
 *  This procedure can be called one of two ways: first, as a selection
 *  callback for the games list, or, second, when it's been detected that
 *  information about the currently selected game has changed.  In the first
 *  case, call_data will be non-NULL and the index of the selected item
 *  becomes the index of our selected game.
 */

	if (call_data != (caddr_t) NULL) {
		p = (XmListCallbackStruct *) call_data;
		selectedGame = p->item_position - 1;
	}

	g = &games[selectedGame];

/*
 *  Free old strings
 */

	for (i=0; i<acount; ++i) {
		if (ateam[i]) {
			XmStringFree (ateam[i]);
		}
	}

	for (i=0; i<bcount; ++i) {
		if (bteam[i]) {
			XmStringFree (bteam[i]);
		}
	}

	acount = bcount = 0;

/*
 *  Build the team lists
 */

	q = g->info.pinfo.pinfo_val;
	for (i=0; i < g->info.pinfo.pinfo_len; ++i, ++q) {
		sprintf (s, "%s (%s) on %s ", q->name,
			q->aircraft, q->display);
		if (q->flags & AFLAG_TEAM1) {
			ateam[acount++] = XmStringCreateSimple (s);
		}
		else	{
			bteam[bcount++] = XmStringCreateSimple (s);
		}
	}

/*
 *  The following forces an empty player list to be at least this wide
 */

#ifdef notdef
	ateam[acount++] =
		XmStringCreateSimple("                                ");
	bteam[bcount++] =
		XmStringCreateSimple("                                ");
#endif

/*
 *  Update the player lists
 */

	n = 0;
	XtSetArg (args[n], XmNitems, ateam); n ++;
	XtSetArg (args[n], XmNitemCount, acount); n ++;
	XtSetValues (widgets[blue_team_id], args, n);

	n = 0;
	XtSetArg (args[n], XmNitems, bteam); n ++;
	XtSetArg (args[n], XmNitemCount, bcount); n ++;
	XtSetValues (widgets[red_team_id], args, n);

/*
 *  Insure that the red and blue team lists have equal widths.
 */
 
	XtVaGetValues (widgets[red_team_id],
		XmNwidth,	&red_width,
		NULL);
	XtVaGetValues (widgets[blue_team_id],
		XmNwidth,	&blue_width,
		NULL);

/*
 *  A special case, if both player lists are empty make them at least 150
 *  pixel wide
 */

	if (acount == 0 && bcount == 0) {
		blue_width = 150;
		XtVaSetValues (widgets[blue_team_id],
			XmNwidth,	blue_width,
			NULL);
	}

	if (blue_width != red_width) {
		if (blue_width > red_width) {
			XtVaSetValues (widgets[red_team_id],
				XmNwidth,	blue_width,
				NULL);
		}
		else {
			XtVaSetValues (widgets[blue_team_id],
				XmNwidth,	red_width,
				NULL);
		}
	}

/*
 *  Build the aircraft list
 */

	for (i=0; i<MAXAIRCRAFT; ++i) {
		if (aircraftStrings[i]) {
			XmStringFree (aircraftStrings[i]);
		}
	}

	if ((aircraftCount = g->info.ainfo.ainfo_len) > MAXAIRCRAFT) {
		aircraftCount = MAXAIRCRAFT;
	}

	for (i=0; i<aircraftCount; ++i) {
		sprintf (s, "%s",
			g->info.ainfo.ainfo_val[i].desc);
		aircraftStrings[i] = XmStringCreateSimple (s);
	}

	n = 0;
	XtSetArg (args[n], XmNitems, aircraftStrings); n ++;
	XtSetArg (args[n], XmNitemCount, aircraftCount); n ++;
	XtSetValues (widgets[aircraft_list_id], args, n);

}

void set_window_title (s)
char	*s;
{
	char	newtitle[MAX_TITLE_LEN];

	strcpy (newtitle, wintitle);
	strcat (newtitle, " - ");
	strcat (newtitle, s);
	XStoreName (XtDisplay(toplevel), XtWindow(toplevel), newtitle);
}

void fatal_error (s)
char	*s;
{
	fprintf (stderr, "%s\n", s);
	exit (1);
}

Widget
CreateHelp (parent) 
Widget		parent;
{
	Widget		button;
	Widget		message_box;
	Arg		args[MAX_ARGS];
	register int	n;

	static char	message[BUFSIZ];
	XmString	title_string = NULL;
	XmString	message_string = NULL;
	XmString	button_string = NULL;

	sprintf (message, "\
Welcome to ACM\n\
The Aerial Combat Simulation for X11\n\
\n\
This program starts by searching the network for acm games\n\
that are already running.  A selection box will display\n\
information about the games that it finds.  You can enter\n\
one of those games by clicking on the game of your choice\n\
and pressing \"OK\".\n\
\0");

	message_string = XmStringCreateLtoR (message, XmSTRING_DEFAULT_CHARSET);
	button_string = XmStringCreateSimple ("Continue");
	title_string = XmStringCreateSimple ("acm help");

	n = 0;
	XtSetArg (args[n], XmNdialogTitle, title_string);  n++;
	XtSetArg (args[n], XmNokLabelString, button_string);  n++;
	XtSetArg (args[n], XmNmessageString, message_string);  n++;
	message_box = XmCreateMessageDialog (parent, "help_dialog", args, n);

	button = XmMessageBoxGetChild (message_box, XmDIALOG_CANCEL_BUTTON);
	XtUnmanageChild (button);
	button = XmMessageBoxGetChild (message_box, XmDIALOG_HELP_BUTTON);
	XtUnmanageChild (button);

	if (title_string)
		XtFree (title_string);
	if (message_string)
		XtFree (message_string);
	if (button_string)
		XtFree (button_string);

	return (message_box);
}

void
InstallNewGameInformation (gp)
GameInfo *gp;
{
	Arg	args[MAX_ARGS];
	GameInfo *p;
	register int i, cnt = 0, n;
	char	s[256];

/*
 *  Build a table of Motif strings to be used as the game selection list
 */

	for (i = 0, p = &games[0]; i<MAXGAMES; ++i, ++p) {

		if (p->open != -1) {
			if (gameName[cnt] != NULL)
				XmStringFree(gameName[i]);

			switch (p->info.pinfo.pinfo_len) {

			case 0:
				sprintf (s, "%s    (no players)",
					p->info.game_name);
				break;
			case 1:
				sprintf (s, "%s    (1 player)",
					p->info.game_name);
				break;
			default:
				sprintf (s, "%s    (%d players)",
					p->info.game_name,
					p->info.pinfo.pinfo_len);
			}

			gameName[cnt] = XmStringCreateSimple (s);
			++ cnt;

/*
 *  If we're updating information about the currently selected game,
 *  install updated player and aircraft lists.
 */

			if (i == selectedGame) {
				record_game_selection (widgets[game_list_id],
					(caddr_t) NULL, (caddr_t) NULL);
			}	
		}
	}

/*
 *  Install the new game list
 */

	n = 0;
	XtSetArg (args[n], XmNlistItems, (XmStringTable) gameName); n ++;
	XtSetArg (args[n], XmNlistItemCount, cnt); n ++;
	XtSetValues (widgets[game_selection_id], args, n);

}

void
cerror (s)
char	*s;
{
	Arg	args[4];
	int	n;

	if (cerrorMessage != NULL)
		XmStringFree (cerrorMessage);

	cerrorMessage = XmStringCreateLtoR (s, XmSTRING_DEFAULT_CHARSET);

	n = 0;
	XtSetArg (args[n], XmNmessageString, cerrorMessage); n ++;
	XtSetValues (widgets[error_id], args, n);

	XtManageChild (widgets[error_id]);

}

void
PostMessage (s)
char	*s;
{
	Arg	args[4];
	int	n;

	if (lastMessage != NULL)
		XmStringFree (lastMessage);

	lastMessage = XmStringCreateLtoR (s, XmSTRING_DEFAULT_CHARSET);

	n = 0;
	XtSetArg (args[n], XmNmessageString, lastMessage); n ++;
	XtSetValues (widgets[message_id], args, n);

	XtManageChild (widgets[message_id]);

}

Boolean
PerformUpdate (client_data)
XtPointer	client_data;
{
	acm_player_input	info;
	acm_player_update	*update;
	craft			*c;
	int			i;

	if (player_id == -1) {
		return False;
	}
	
	GetStick();

	info.session = session;
	c = &ptbl[player_id];
	info.Se = c->Se + c->SeTrim;
	info.Sr = c->Sr;
	info.Sa = c->Sa;
	info.throttle = c->throttle;
	info.flags = c->flags & FL_INPUT_MASK;

/*
 *  Pack up any commands received from the user ...
 */
 
	info.command.command_len = c->num_commands;
	if (info.command.command_len > 0) {
	    info.command.command_val = (acm_command *)
		malloc (info.command.command_len * sizeof(acm_command));
	    for (i=0; i<c->num_commands; ++i) {
		info.command.command_val[i] = *(c->commands[i]);
	    }
	}

	for (i=0; i<c->num_commands; ++i) {
		free ((char *) c->commands[i]);
	}
	if (c->num_commands > 0) {
		free ((char *) c->commands);
	}
	c->num_commands = 0;

	if ((update = acmsetplayerinput_1 (&info, clnt)) != NULL) {

		for (i=0, c=ptbl; i<MAXPLAYERS; ++i, ++c) {
			c->prevSg = c->Sg;
			c->type = CT_FREE;
		}

		for (i=0, c=mtbl; i<MAXPROJECTILES; ++i, ++c) {
			c->prevSg = c->Sg;
			c->type = CT_FREE;
		}

		c = &ptbl[player_id];

		ImportPlayerUpdate (update, c);
#ifdef notdef
		printf ("objects %d\n", update->object.object_len);
		printf ("object	1 type %d id %d\n",
			update->object.object_val[0].type,
			update->object.object_val[0].id);
#endif
		xdr_free (xdr_acm_player_update, update);

		if (c->type == CT_FREE && player_id != -1) {
			ExitGame (game_over);
		}

		if (player_id == -1) {
			return False;
		}
		
		UpdateClientDatabase();

		Render ();

		++ update_count;

	}
	else {
		ExitGame ("The game server seems to have died.");
	}
	++ updates_processed;
	return False;
}

