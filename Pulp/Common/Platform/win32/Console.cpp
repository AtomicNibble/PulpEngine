#include "EngineCommon.h"

#pragma hdrstop

#include "Console.h"
#include "Util\LastError.h"

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <fstream>

#include <ICore.h>

// #include "resource.h"

#include "resource.h"

X_NAMESPACE_BEGIN(core)

namespace {

	COORD GetCoord(unsigned int x, unsigned int y)
	{
		COORD cord;

		cord.X = safe_static_cast<short,uint>(x);
		cord.Y = safe_static_cast<short,uint>(y);

		return cord;
	}

	BOOL _SetConsoleIcon( HICON hIcon )
	{
		typedef BOOL (WINAPI *SetConsoleIcon_t)(HICON handle);

		SetConsoleIcon_t pSetConsoleIcon;

		
		HMODULE hDll = GetModuleHandleA( "kernel32" );
		if( hDll )
		{
			pSetConsoleIcon = (SetConsoleIcon_t)GoatGetProcAddress(hDll, "SetConsoleIcon");
			if( pSetConsoleIcon )
				return pSetConsoleIcon( hIcon );
		}
		return false;
	}

	void SetConsoleIcon( lastError::Description& Dsc, WORD ID )
	{
		HICON icon = (HICON)::LoadImage( 
					::GetModuleHandle(NULL), 
					MAKEINTRESOURCE( ID ), 
					IMAGE_ICON, 
					0, 
					0, 
					0 
				);

		if( !icon )
		{
			X_ERROR( "Console", "Cannot load console icon. ID: %i Error: %s", ID, lastError::ToString( Dsc ) );
		}
		else if( !_SetConsoleIcon( icon ) )
		{
			X_ERROR( "Console", "Failed to seticon on console. Error: %s", lastError::ToString( Dsc ) );
		}
	}

};

Console::Console(const wchar_t* title) :
m_stdout(nullptr),
m_stdin(nullptr),
m_stderr(nullptr)
{
//	int hConHandle;
//	HANDLE lStdHandle;

	lastError::Description Dsc;

	// allocate a console for this app
	if (!AllocConsole())
	{
		X_ERROR("Console", "Cannot allocate new console. Error: %s", lastError::ToString(Dsc));
		return;
	}

	m_window = GetConsoleWindow();

	if( m_window == NULL )
	{
		X_ERROR( "Console", "Cannot retrieve console window. Error: %s", lastError::ToString( Dsc ) );
		return;
	}

	SetConsoleIcon(Dsc, IDI_ENGINE_LOGO);

	m_console = GetStdHandle(STD_OUTPUT_HANDLE);
	m_consoleInput = GetStdHandle(STD_INPUT_HANDLE);

	if( m_consoleInput == INVALID_HANDLE_VALUE )
	{
		X_ERROR( "Console", "Cannot retrieve console input handle. Error: %s", lastError::ToString( Dsc ) );
		return;
	}

	if( SetConsoleMode( m_consoleInput, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT ) == 0 )
	{
		X_ERROR( "Console", "Cannot set console input mode. Error: %s", lastError::ToString( Dsc ) );
		return;
	}

	/*
	lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	m_stdout = _fdopen( hConHandle, "w" );
	*stdout = *m_stdout;
//	setvbuf( stdout, NULL, _IONBF, 0 );

	lStdHandle = GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	m_stdin = _fdopen( hConHandle, "r" );
	*stdin = *m_stdin;
//	setvbuf( stdin, NULL, _IONBF, 0 );


	lStdHandle = GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	m_stderr = _fdopen( hConHandle, "w" );
	*stderr = *m_stderr;
//	setvbuf( stderr, NULL, _IONBF, 0 );
	*/
	SetTitle( title );

//	std::ios::sync_with_stdio();
}

/// Frees all resources.
Console::~Console(void)
{
	lastError::Description Dsc;

	if (m_stdout) {
		if (fclose(m_stdout) == EOF)
			X_ERROR("Console", "Cannot close stdout handle. Error: %s", lastError::ToString(Dsc));
	}
	if (m_stdout) {
		if (fclose(m_stdin) == EOF)
			X_ERROR("Console", "Cannot close stdin handle. Error: %s", lastError::ToString(Dsc));
	}
	if (m_stdout) {
		if (fclose(m_stderr) == EOF)
			X_ERROR("Console", "Cannot close stderr handle. Error: %s", lastError::ToString(Dsc));
	}

	if( !FreeConsole() )
	{
		X_ERROR( "Console", "Cannot free console. Error: %s", lastError::ToString( Dsc ) );
	}

}

/// Sets the console title.
void Console::SetTitle(const wchar_t* title)
{
	SetConsoleTitleW( title );
}

/// \brief Sets the console window size and number of lines stored internally, in character units.
void Console::SetSize(unsigned int windowWidth, unsigned int windowHeight, unsigned int numLines)
{
	lastError::Description Dsc;
	CONSOLE_SCREEN_BUFFER_INFO Info;

	WORD width = safe_static_cast<short,uint>(windowWidth);
	WORD height = safe_static_cast<short,uint>(windowHeight);
	WORD lines = safe_static_cast<short,uint>(numLines);

	if( !GetConsoleScreenBufferInfo( m_console, &Info ) )
	{
		X_ERROR( "Console", "Cannot get console buffer info. Error: %s", lastError::ToString( Dsc ) );
		return;
	}

	// only increase buffer width don't shrink.
	if( Info.dwSize.X < width  )
	{
		Info.dwSize.X = Max<short>( safe_static_cast<short,uint>( windowWidth ), safe_static_cast<short,int>( GetSystemMetrics( SM_CXMIN ) ) );

		if( !SetConsoleScreenBufferSize( m_console, Info.dwSize ) )
		{
			X_ERROR( "Console", "Cannot set console buffer width. Error: %s", lastError::ToString( Dsc ) );
		}
	}

	// only increase buffer height don't shrink.
	if( Info.dwSize.Y < lines )
	{
		Info.dwSize.Y = Max<short>( lines, safe_static_cast<short,int>( GetSystemMetrics( SM_CYMIN ) ) );

		if( !SetConsoleScreenBufferSize( m_console, Info.dwSize ) )
		{
			X_ERROR( "Console", "Cannot set console buffer height. Error: %s", lastError::ToString( Dsc ) );
		}
	}

	// confirm buffer sizes.
	GetConsoleScreenBufferInfo( m_console, &Info );

	width = Min( width, safe_static_cast<WORD,SHORT>( Info.dwSize.X ) ) - 1;
	height = Min( height, safe_static_cast<WORD,SHORT>( Info.dwSize.Y ) ) - 1;
	
	SMALL_RECT windowSize = {
		0, 
		0, 
		width, 
		height
	};

	if( !SetConsoleWindowInfo( m_console, 1, &windowSize ) )
	{
		X_ERROR( "Console", "Cannot set console window width. Error: %s", lastError::ToString( Dsc ) );
	}

}


/// Sets the cursor position inside the console, in character units.
void Console::SetCursorPosition(unsigned int x, unsigned int y)
{
	if( !SetConsoleCursorPosition( m_console, GetCoord( x, y ) ) )
	{
		lastError::Description Dsc;
		X_ERROR( "Console", "Cannot set console cursor position. Error: %s", lastError::ToString( Dsc ) );
	}
}

/// Moves the console window to a certain position, in pixel units.
void Console::MoveTo(int x, int y)
{
	RECT rec;
	GetWindowRect( m_window, &rec );

	int nHeight = rec.bottom - rec.top;
	int nWidth = rec.right - rec.left;

	if( !MoveWindow( m_window, x, y, nWidth, nHeight, FALSE ) )
	{
		lastError::Description Dsc;
		X_ERROR( "Console", "Cannot move console window. Error: %s", lastError::ToString( Dsc ) );
	}
}

/// Moves the console window to a certain position, in pixel units.
void Console::MoveTo(const Vec2i& position)
{
	MoveTo( position.x, position.y );
}

/// Aligns the console window to any xRect.
void Console::AlignTo(const Rect& xRect, Alignment alignment)
{
	MoveTo( GetRect().Align( xRect, alignment ).getUpperLeft() );
}

/// Returns the console xRect, in pixel units.
Console::Rect Console::GetRect(void) const
{
	RECT rec;
	GetWindowRect( m_window, &rec );

//	int nHeight = rec.bottom - rec.top;
//	int nWidth = rec.right - rec.left;

	return Rect( rec.left, rec.top, rec.right, rec.bottom );
}

/// \brief Tries to read a key from the console, and returns its virtual key-code.
/// \details Returns 0 if no key has been pressed.
char Console::ReadKey(void) const
{
	lastError::Description Dsc;
	DWORD NumEvents;

	if( GetNumberOfConsoleInputEvents( m_consoleInput, &NumEvents ) )
	{
		if( NumEvents > 0 )
		{
			INPUT_RECORD InputInfo;

			if( ReadConsoleInput( m_consoleInput, &InputInfo, 1, &NumEvents ) )
			{
				if( InputInfo.EventType == KEY_EVENT )
				{
					return safe_static_cast<char,WORD>( InputInfo.Event.KeyEvent.wVirtualKeyCode );
				}
			}
			else
			{
				X_ERROR( "Console", "Cannot read console input. Error: %s", lastError::ToString( Dsc ) );
			}
		}
	}
	else
	{
		X_ERROR( "Console", "Cannot read key. Error: %s", lastError::ToString( Dsc ) );
	}

	return 0;
}

/// Shows/hides the console window.
void Console::Show(bool show)
{
	ShowWindow( m_window, (show ? SW_SHOW : SW_HIDE) );
}


void Console::RedirectSTD(void)
{
	int hConHandle;
	HANDLE lStdHandle;

	lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	m_stdout = _fdopen( hConHandle, "w" );
	*stdout = *m_stdout;
	//	setvbuf( stdout, NULL, _IONBF, 0 );

	lStdHandle = GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	m_stdin = _fdopen( hConHandle, "r" );
	*stdin = *m_stdin;
	//	setvbuf( stdin, NULL, _IONBF, 0 );


	lStdHandle = GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	m_stderr = _fdopen( hConHandle, "w" );
	*stderr = *m_stderr;
	//	setvbuf( stderr, NULL, _IONBF, 0 );

	std::ios::sync_with_stdio();
}

X_NAMESPACE_END