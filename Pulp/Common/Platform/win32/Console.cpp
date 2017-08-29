#include "EngineCommon.h"

#include "Console.h"
#include "Util\LastError.h"

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <fstream>
#include <conio.h>

#include <ICore.h>

#include <Platform\Module.h>

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

		
		core::Module::Handle hDll = core::Module::Load( "kernel32" );
		if( hDll )
		{
			pSetConsoleIcon = (SetConsoleIcon_t)core::Module::GetProc(hDll, "SetConsoleIcon");
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
stdout_(nullptr),
stdin_(nullptr),
stderr_(nullptr)
{
	lastError::Description Dsc;

	// allocate a console for this app
	if (!AllocConsole())
	{
		X_ERROR("Console", "Cannot allocate new console. Error: %s", lastError::ToString(Dsc));
		return;
	}

	window_ = GetConsoleWindow();

	if( window_ == NULL )
	{
		X_ERROR( "Console", "Cannot retrieve console window. Error: %s", lastError::ToString( Dsc ) );
		return;
	}

	SetConsoleIcon(Dsc, IDI_ENGINE_LOGO);

	console_ = GetStdHandle(STD_OUTPUT_HANDLE);
	consoleInput_ = GetStdHandle(STD_INPUT_HANDLE);

	if( consoleInput_ == INVALID_HANDLE_VALUE )
	{
		X_ERROR( "Console", "Cannot retrieve console input handle. Error: %s", lastError::ToString( Dsc ) );
		return;
	}

	if( SetConsoleMode( consoleInput_, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT ) == 0 )
	{
		X_ERROR( "Console", "Cannot set console input mode. Error: %s", lastError::ToString( Dsc ) );
		return;
	}

	SetTitle( title );
}

/// Frees all resources.
Console::~Console(void)
{
	lastError::Description Dsc;
	/*
	if (stdout_) {
		if (fclose(stdout_) == EOF)
			X_ERROR("Console", "Cannot close stdout handle. Error: %s", lastError::ToString(Dsc));
	}
	if (stdout_) {
		if (fclose(stdin_) == EOF)
			X_ERROR("Console", "Cannot close stdin handle. Error: %s", lastError::ToString(Dsc));
	}
	if (stdout_) {
		if (fclose(stderr_) == EOF)
			X_ERROR("Console", "Cannot close stderr handle. Error: %s", lastError::ToString(Dsc));
	}
	*/
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

	SHORT width = safe_static_cast<SHORT,uint>(windowWidth);
	SHORT height = safe_static_cast<SHORT,uint>(windowHeight);
	SHORT lines = safe_static_cast<SHORT,uint>(numLines);

	if( !GetConsoleScreenBufferInfo( console_, &Info ) )
	{
		X_ERROR( "Console", "Cannot get console buffer info. Error: %s", lastError::ToString( Dsc ) );
		return;
	}

	// only increase buffer width don't shrink.
	if( Info.dwSize.X < width  )
	{
		Info.dwSize.X = Max<short>( safe_static_cast<short,uint>( windowWidth ), safe_static_cast<short,int>( GetSystemMetrics( SM_CXMIN ) ) );

		if( !SetConsoleScreenBufferSize( console_, Info.dwSize ) )
		{
			X_ERROR( "Console", "Cannot set console buffer width. Error: %s", lastError::ToString( Dsc ) );
		}
	}

	// only increase buffer height don't shrink.
	if( Info.dwSize.Y < lines )
	{
		Info.dwSize.Y = Max<short>( lines, safe_static_cast<short,int>( GetSystemMetrics( SM_CYMIN ) ) );

		if( !SetConsoleScreenBufferSize( console_, Info.dwSize ) )
		{
			X_ERROR( "Console", "Cannot set console buffer height. Error: %s", lastError::ToString( Dsc ) );
		}
	}

	// confirm buffer sizes.
	GetConsoleScreenBufferInfo( console_, &Info );

	width = Min( width, static_cast<SHORT>( Info.dwSize.X ) ) - 1;
	height = Min( height, static_cast<SHORT>( Info.dwSize.Y ) ) - 1;
	
	SMALL_RECT windowSize = {
		0, 
		0, 
		width, 
		height
	};

	if( !SetConsoleWindowInfo( console_, 1, &windowSize ) )
	{
		X_ERROR( "Console", "Cannot set console window width. Error: %s", lastError::ToString( Dsc ) );
	}

}


/// Sets the cursor position inside the console, in character units.
void Console::SetCursorPosition(unsigned int x, unsigned int y)
{
	if( !SetConsoleCursorPosition( console_, GetCoord( x, y ) ) )
	{
		lastError::Description Dsc;
		X_ERROR( "Console", "Cannot set console cursor position. Error: %s", lastError::ToString( Dsc ) );
	}
}

/// Moves the console window to a certain position, in pixel units.
void Console::MoveTo(int x, int y)
{
	RECT rec;
	GetWindowRect( window_, &rec );

	int nHeight = rec.bottom - rec.top;
	int nWidth = rec.right - rec.left;

	if( !MoveWindow( window_, x, y, nWidth, nHeight, FALSE ) )
	{
		lastError::Description Dsc;
		X_ERROR( "Console", "Cannot move console window. Error: %s", lastError::ToString( Dsc ) );
	}
}

/// Moves the console window to a certain position, in pixel units.
void Console::MoveTo(const Position& position)
{
	MoveTo( position.x, position.y );
}

/// Aligns the console window to any xRect.
void Console::AlignTo(const Rect& xRect, AlignmentFlags alignment)
{
	MoveTo( GetRect().Align( xRect, alignment ).getUpperLeft() );
}

/// Returns the console xRect, in pixel units.
Console::Rect Console::GetRect(void) const
{
	RECT rec;
	GetWindowRect( window_, &rec );

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

	if( GetNumberOfConsoleInputEvents( consoleInput_, &NumEvents ) )
	{
		if (NumEvents > 0)
		{
			for (DWORD i = 0; i < NumEvents; i++)
			{
				INPUT_RECORD InputInfo;
				DWORD NumEventsRead;

				if (ReadConsoleInputW(consoleInput_, &InputInfo, 1, &NumEventsRead))
				{
					X_ASSERT(NumEventsRead == 1, "Should only get one event")(NumEventsRead);

					if (NumEventsRead >= 1 &&InputInfo.EventType == KEY_EVENT)
					{
						return safe_static_cast<char, WORD>(InputInfo.Event.KeyEvent.wVirtualKeyCode);
					}
				}
				else
				{
					X_ERROR("Console", "Cannot read console input. Error: %s", lastError::ToString(Dsc));
				}
			}
		}
	}
	else
	{
		X_ERROR( "Console", "Cannot read key. Error: %s", lastError::ToString( Dsc ) );
	}

	return 0;
}

char Console::ReadKeyBlocking(void) const
{
	INPUT_RECORD InputInfo;
	DWORD NumEventsRead;

	while (ReadConsoleInputW(consoleInput_, &InputInfo, 1, &NumEventsRead))
	{
		X_ASSERT(NumEventsRead == 1, "Should only get one event")(NumEventsRead);

		if (NumEventsRead >= 1 && InputInfo.EventType == KEY_EVENT)
		{
			return safe_static_cast<char, WORD>(InputInfo.Event.KeyEvent.wVirtualKeyCode);
		}
	}

	lastError::Description Dsc;
	X_ERROR("Console", "Cannot read console input. Error: %s", lastError::ToString(Dsc));
	return 0;
}

/// Shows/hides the console window.
void Console::Show(bool show)
{
	ShowWindow( window_, (show ? SW_SHOW : SW_HIDE) );
}


void Console::RedirectSTD(void)
{
#if _MSC_FULL_VER >= 190023026
	// shit got broken in vs2015 toolset v140
	// https://connect.microsoft.com/VisualStudio/Feedback/Details/1924921

	for (auto &file : { stdout, stderr }) {
		FILE* pFile = nullptr;
		freopen_s(&pFile, "CONOUT$", "w", file);
		setvbuf(file, nullptr, _IONBF, 0);
	}

#else
	int hConHandle;
	HANDLE lStdHandle;

	lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	stdout_ = _fdopen( hConHandle, "w" );
	*stdout = *stdout_;
	//	setvbuf( stdout, NULL, _IONBF, 0 );

	lStdHandle = GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	stdin_ = _fdopen( hConHandle, "r" );
	*stdin = *stdin_;
	//	setvbuf( stdin, NULL, _IONBF, 0 );


	lStdHandle = GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	stderr_ = _fdopen( hConHandle, "w" );
	*stderr = *stderr_;
	//	setvbuf( stderr, NULL, _IONBF, 0 );

	std::ios::sync_with_stdio();
#endif // !_MSC_FULL_VER
}

void Console::PressToContinue(void) const
{
	if (gEnv && gEnv->pLog) {
		X_LOG0("Console", "Press to continue");
	}

	(void)_getch();
}

X_NAMESPACE_END