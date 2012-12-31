// $Id: MouseDrag.cpp 102 2009-01-09 20:42:45Z osmith@CRS $
// Allows the user to drag the window by its contents
// e.g. if we make the window borderless.

// Gets set to true when a mouse-down event is sent
static bool g_dragging = false ;
// The offset at which the user depressed the button
static POINT g_offset ;

// Handle the button down event
static void _onButtonDown(HWND hWnd, LPARAM lParam)
{
	g_dragging = true ;
	g_offset.x = LOWORD(lParam) ;
	g_offset.y = HIWORD(lParam) ;
	ClientToScreen(hWnd, &g_offset) ;
	SetCapture(hWnd) ;
}

// Handle button up
static void _onButtonUp()
{
	g_dragging = false ;
	ReleaseCapture() ;
}

// Handle the mouse movement
static void _onMouseMove(HWND hWnd, LPARAM lParam)
{
	if ( g_dragging == false )
		return ;

	POINT pos ;
	pos.x = LOWORD(lParam) ;
	pos.y = HIWORD(lParam) ;
	ClientToScreen(hWnd, &pos) ;

	int offsetx = pos.x - g_offset.x ;
	int offsety = pos.y - g_offset.y ;

	RECT r ;
	GetWindowRect(hWnd, &r) ;

	int width = r.right - r.left ;
	int height = r.bottom - r.top ;

	g_offset = pos ;

	MoveWindow(hWnd, r.left + offsetx, r.top + offsety, width, height, TRUE) ;
}

// Processing functions.
void MouseDragProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_INITDIALOG:
			g_dragging = false ;
		break ;
		case WM_CREATE:
			g_dragging = false ;
		break ;
		case WM_LBUTTONDOWN:
			_onButtonDown(hWnd, lParam) ;
		break ;
		case WM_LBUTTONUP:
			_onButtonUp() ;
		break ;
		case WM_MOUSEMOVE:
			_onMouseMove(hWnd, lParam) ;
		break ;
	}
}

