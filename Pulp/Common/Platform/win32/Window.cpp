#include "EngineCommon.h"

#include "Window.h"
#include "CustomFrame.h"

#include "Util\LastError.h"

#include <ICore.h>

// #include "resource.h"

X_NAMESPACE_BEGIN(core)

namespace 
{
	static const char* g_EngineName = X_ENGINE_NAME"Engine";

	BOOL g_ClassRegisterd = FALSE;
	
	// We have a pre and final winproc.
	// The pre one waits untill we can get the class pointer.
	// Then switches to a lightweight winproc as each window is only created once.
	// and we no longer need to do checks for msgs.
	LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{	
		LONG_PTR data = GetWindowLongPtr( hWnd, GWLP_USERDATA ); // GetWindowLongPtr required for 64

		return reinterpret_cast<xWindow*>(data)->WndProc(hWnd, msg, wParam, lParam);
	}

	LRESULT CALLBACK PreWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{		
		if( msg == WM_NCCREATE ) 
		{
			LPCREATESTRUCT pInfo = reinterpret_cast<LPCREATESTRUCT>(lParam);
			xWindow* pClass = reinterpret_cast<xWindow*>(pInfo->lpCreateParams);

			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pClass) );
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc) );

			return pClass->WndProc(hWnd, msg, wParam, lParam);
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}


}

Recti Convert(const RECT& r)
{
	return Recti(r.left, r.top, r.right, r.bottom);
}


uint32_t xWindow::s_numwindows = 0;

void xWindow::RegisterClass(void)
{
	if (!g_ClassRegisterd)
	{
		// Register the class
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = /*CS_HREDRAW | CS_VREDRAW |*/ CS_DBLCLKS | CS_OWNDC;
		wcex.lpfnWndProc = PreWndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = GetModuleHandle(NULL);
		wcex.hIcon = 0;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = 0;
		wcex.lpszMenuName = 0;
		wcex.lpszClassName = g_EngineName;
		wcex.hIconSm = 0;

		RegisterClassEx(&wcex);

		g_ClassRegisterd = TRUE;
	}
	s_numwindows++;
}


void xWindow::UnRegisterClass(void)
{
	s_numwindows--;
	if (s_numwindows == 0 && g_ClassRegisterd) {
		UnregisterClass(g_EngineName, GetModuleHandle(NULL));
		g_ClassRegisterd = FALSE;
	}
}


/// --------------------------------------------------------------------------------------------------------

/* 
xWindow::xWindow(const char* const Title, int x, int y, int width, int height, Mode::Enum mode) : m_NumMsgs(0), m_InputCheck(FALSE)
	, m_pFrame( nullptr )
{
	CreateWindow(Title, x, y, width, height, mode);
}
*/

xWindow::xWindow() : 
m_NumMsgs(0),
m_HideClientCursor(FALSE),
m_pFrame(nullptr)
{
	RegisterClass();
}


xWindow::~xWindow(void)
{
	Destroy();
	UnRegisterClass();
	X_DELETE(m_pFrame, gEnv->pArena);
}


/// --------------------------------------------------------------------------------------------------------


xWindow::Notification::Enum xWindow::PumpMessages(void) const
{
	MSG msg;

	// need two for WM_INPUT goat shiz.
	while (PeekMessage(&msg, NULL, 0, WM_INPUT - 1, PM_REMOVE))
	{
		m_NumMsgs++;

		if (msg.message == WM_CLOSE)
			return Notification::CLOSE;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	while (PeekMessage(&msg, NULL, WM_INPUT + 1, 0xffffffff, PM_REMOVE))
	{
		m_NumMsgs++;

		if(msg.message == WM_CLOSE)
			return Notification::CLOSE;

		TranslateMessage(&msg);
		DispatchMessage(&msg); 
	}

	return Notification::NONE;
}


/// --------------------------------------------------------------------------------------------------------

void xWindow::CustomFrame(bool val)
{
	if (val)
	{
		if (m_pFrame == nullptr)
			m_pFrame = X_NEW( xFrame, gEnv->pArena, "Win32CustomFrame");
	}
	else
	{
		if (m_pFrame != nullptr)
		{
			X_DELETE_AND_NULL(m_pFrame, gEnv->pArena);


			RedrawWindow(m_window, NULL, NULL, RDW_FRAME | RDW_UPDATENOW | RDW_NOCHILDREN);
			::InvalidateRect(m_window, NULL, TRUE);
			::UpdateWindow(m_window);
		}
	}
}

void xWindow::MoveTo(int x, int y)
{
	RECT rect;
	GetWindowRect( m_window, &rect );
	MoveWindow( m_window, x, y, rect.right - rect.left, rect.bottom - rect.top, FALSE );
}

void xWindow::MoveTo(const Position& position)
{
	MoveTo( position.x, position.y );
}


void xWindow::AlignTo(const Recti& Rect, xAlign alignment)
{
	RECT _Rect;
	GetWindowRect( m_window, &_Rect );

	Recti rect = Convert(_Rect);
	rect.Align(Rect, alignment);

	MoveTo(rect.x1, rect.y1);
}


Recti xWindow::GetRect(void) const
{
	RECT rect;
	GetWindowRect( m_window, &rect );

	return Convert(rect);
}

Recti xWindow::GetClientRect(void) const
{
	RECT rect;
	::GetClientRect( m_window, &rect );

	return Convert(rect);
}

/// --------------------------------------------------------------------------------------------------------

// Returns the xRect of the primary display monitor, not overlapping the taskbar.
Recti xWindow::GetPrimaryRect(void)
{
	RECT rect;
	SystemParametersInfo( SPI_GETWORKAREA, NULL, &rect, NULL );

	return Convert(rect);
}

// Returns the xRect of the complete desktop area, spanning all monitors and overlapping the taskbar.
Recti xWindow::GetDesktopRect(void)
{
	return Recti(
		GetSystemMetrics( SM_XVIRTUALSCREEN ),
		GetSystemMetrics( SM_YVIRTUALSCREEN ),
		GetSystemMetrics( SM_CXVIRTUALSCREEN ),
		GetSystemMetrics( SM_CYVIRTUALSCREEN )	
	);
}

/// --------------------------------------------------------------------------------------------------------


bool xWindow::Create(const char* const Title, int x, int y, int width, int height, xWindow::Mode::Enum mode)
{
	lastError::Description Dsc;

	if( !g_ClassRegisterd ) {
		X_FATAL( "Window", "You cannot create a window before xlib has been started.", Title, lastError::ToString( Dsc ) );
		return false;
	}

	if( mode != xWindow::Mode::FULLSCREEN )
	{
		// Make the size the client size.
		RECT Rect;

		Rect.left = Rect.top = 0;
		Rect.right = width;
		Rect.bottom = height;

		AdjustWindowRect( &Rect, mode, FALSE );

		width = safe_static_cast<int,LONG>( Rect.right - Rect.left );
		height = safe_static_cast<int,LONG>( Rect.bottom - Rect.top );
	}

	m_window = CreateWindowEx( 
		0, 
		g_EngineName, 
		Title, 
		mode, 
		x, y, 
		width, height, 
		GetDesktopWindow(), 
		0, 
		GetModuleHandle( NULL ),
		this // required for winproc
		);


	HICON hIcon = NULL; //  LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ENGINE_LOGO));
	SendMessage(m_window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	SendMessage(m_window, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

	X_ERROR_IF( m_window == NULL, "Window", "Failed to create window. Title: '%s'. Error: %s", Title, lastError::ToString( Dsc ) );

	return m_window != NULL;
}


/// --------------------------------------------------------------------------------------------------------

LRESULT xWindow::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch (msg)
	{
		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT && m_HideClientCursor)
			{
				SetCursor(NULL);
				return TRUE;
			}
			break;
		case WM_ACTIVATE:
			gEnv->pCore->GetCoreEventDispatcher()->OnCoreEvent(
				CoreEvent::CHANGE_FOCUS, wParam, lParam);

			if (wParam != WA_INACTIVE) {
				gEnv->pCore->GetCoreEventDispatcher()->OnCoreEvent(
					CoreEvent::ACTIVATE, wParam, lParam);
			}
		break;

		case WM_SIZE:
			gEnv->pCore->GetCoreEventDispatcher()->OnCoreEvent(
				CoreEvent::RESIZE, wParam, lParam);
		break;
		case WM_MOVE:
			gEnv->pCore->GetCoreEventDispatcher()->OnCoreEvent(
				CoreEvent::MOVE, wParam, lParam);
			break;
	}

	if( m_pFrame ) 
		return m_pFrame->DrawFrame( hWnd, msg, wParam, lParam );		

	return DefWindowProc( hWnd, msg, wParam, lParam );
}



/// --------------------------------------------------------------------------------------------------------

X_NAMESPACE_END