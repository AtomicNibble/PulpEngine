#include "EngineCommon.h"

#include "Platform\Console.h"
#include "LoggerConsoleWritePolicy.h"

#include "Util\LastError.h"

#include <ICore.h>

X_NAMESPACE_BEGIN(core)

namespace {

	enum Cols
	{
		COL_BLACK,
		COL_WHITE,
		COL_GRAY,
		COL_GRAY_LIGHT,

		COL_BLUE_LIGHT,
		COL_GREEN,
		COL_RED,

		COL_YELLOW,
		COL_PURPLE,
		COL_CYAN,

		COL_BLUE,
		COL_GREEN_LIGHT,
		COL_RED_LIGHT,

		COL_ORANGE,
		COL_LIGHTPINK
	};

	X_INLINE unsigned __int16 MakeColor( Cols For, Cols Back )
	{
		return  For + (Back << 4);
	}

	const unsigned int COLOR_TABLE[16] = { 
		RGB(0x10, 0x10, 0x10),
		RGB( 0xff, 0xff, 0xff ), 
		RGB(0x10, 0x10, 0x10),
		RGB( 0x70, 0x70, 0x70 ), 

		RGB(0, 0x66, 0xcc),
		RGB( 0x56, 0xa8, 0x3c ), 
		RGB(0xff, 0, 0),
		RGB( 0xff, 0xff, 0x1c ), 
		RGB( 0xa3, 0x38, 0xff ), 
		RGB( 0, 0xe0, 0xe0 ), 

		RGB(0, 0x66, 0xcc),
		RGB( 0, 0x66, 0 ), 
		RGB(0xcc, 0, 0),

		RGB( 0xcc, 0x70, 0 ), 
		RGB( 0xff, 0xaa, 0x9b ),
		0x000000,  

	};

}


LoggerConsoleWritePolicy::LoggerConsoleWritePolicy(const Console& console) : console_( console.GetNativeConsole() )
{
	
}


/// Initializes a custom color table for the console.
void LoggerConsoleWritePolicy::Init(void)
{
	CONSOLE_SCREEN_BUFFER_INFOEX InfoEx;

	InfoEx.cbSize = sizeof( CONSOLE_SCREEN_BUFFER_INFOEX );


	lastError::Description Dsc;

	if (!GetConsoleScreenBufferInfoEx(console_, &InfoEx))
	{
		X_ERROR( "ConsoleLogger", "Failed get buffer info. Error: %s", lastError::ToString( Dsc ) );
	}

	memcpy( InfoEx.ColorTable, COLOR_TABLE, sizeof( COLOR_TABLE ) );

	++InfoEx.srWindow.Right;
	++InfoEx.srWindow.Bottom;

	if (!SetConsoleScreenBufferInfoEx(console_, &InfoEx))
	{
		X_ERROR( "ConsoleLogger", "Failed to set buffer info. Error: %s", lastError::ToString( Dsc ) );
	}
}

/// Empty implementation.
void LoggerConsoleWritePolicy::Exit(void)
{
	
}

#include <wchar.h>

namespace {

	const unsigned __int16 LOG_COLOR				= MakeColor( COL_WHITE, COL_GRAY ); // COL_WHITE;
	const unsigned __int16 WARNING_COLOR			= MakeColor( COL_WHITE, COL_ORANGE );
	const unsigned __int16 ERROR_COLOR				= MakeColor( COL_RED, COL_GRAY );
	const unsigned __int16 FATAL_ERROR_COLOR		= MakeColor( COL_GRAY, COL_RED );
	const unsigned __int16 ASSERT_COLOR				= MakeColor( COL_RED, COL_BLACK );
	const unsigned __int16 ASSERT_VARIABLE_COLOR	= MakeColor( COL_WHITE, COL_RED );
	const unsigned __int16 CHANNEL_COLOR			= MakeColor( COL_BLUE_LIGHT, COL_GRAY );
	const unsigned __int16 STRING_COLOR				= MakeColor( COL_GREEN, COL_BLACK ); // COL_GREEN;


	void WriteToConsole( HANDLE console, unsigned __int16 color, const char* asciiMsg, unsigned int length, bool colorizeExtended )
	{
		char oemMsg[sizeof(LoggerBase::Line)];

		DWORD msgEnd;
		DWORD NumberOfCharsWritten;
		DWORD SectionSize = 0;
		bool isString = false;
		const char *pOem;

		CharToOem( asciiMsg, oemMsg );	

		pOem = oemMsg;
		msgEnd = (unsigned int)&oemMsg[length];
		if ( oemMsg < &oemMsg[length] )
		{
			do
			{
				if ( SectionSize >= length )
					break;

				if ( *(pOem + SectionSize) == '|' )
				{
					SetConsoleTextAttribute( console, CHANNEL_COLOR );

					NumberOfCharsWritten = 0;
					WriteConsoleA( console, pOem, SectionSize, &NumberOfCharsWritten, 0);

					length -= SectionSize;
					pOem = (char *)pOem + SectionSize;
					SectionSize = 0;
				}


				if ( colorizeExtended && *(pOem + SectionSize) == '\"' )
				{
					if ( isString )
					{
						++SectionSize;
						SetConsoleTextAttribute( console, STRING_COLOR );
					}
					else
					{
						SetConsoleTextAttribute( console, color );
					}	

					NumberOfCharsWritten = 0;
					WriteConsoleA( console, pOem, SectionSize, &NumberOfCharsWritten, 0 );

					length -= SectionSize;
					pOem = (char *)pOem + SectionSize;
					SectionSize = 0;

					isString = (isString == 0);
				}

				++SectionSize;
			}
			while ( (unsigned int)pOem < msgEnd );
		}

		SetConsoleTextAttribute( console, color );
		msgEnd = 0;
		WriteConsoleA( console, pOem, length, &msgEnd, 0 );

		// reset
		SetConsoleTextAttribute(console, LOG_COLOR);
	}

};

/// Writes a log message to the console.
void LoggerConsoleWritePolicy::WriteLog( const LoggerBase::Line& line, uint32_t length )
{
	CriticalSection::ScopedLock lock(cs_);
	
	WriteToConsole(console_, LOG_COLOR, line, length, true);
}

/// Writes a warning message to the console.
void LoggerConsoleWritePolicy::WriteWarning(const LoggerBase::Line& line, uint32_t length)
{
	CriticalSection::ScopedLock lock(cs_);
	
	WriteToConsole(console_, WARNING_COLOR, line, length, false);
}

/// Writes an error message to the console.
void LoggerConsoleWritePolicy::WriteError(const LoggerBase::Line& line, uint32_t length)
{
	CriticalSection::ScopedLock lock(cs_);
	
	WriteToConsole(console_, ERROR_COLOR, line, length, false);
}

/// Writes a fatal error message to the console.
void LoggerConsoleWritePolicy::WriteFatal(const LoggerBase::Line& line, uint32_t length)
{
	CriticalSection::ScopedLock lock(cs_);
	
	WriteToConsole(console_, FATAL_ERROR_COLOR, line, length, false);
}

/// Writes an assert message to the console.
void LoggerConsoleWritePolicy::WriteAssert(const LoggerBase::Line& line, uint32_t length)
{
	CriticalSection::ScopedLock lock(cs_);
	
	WriteToConsole(console_, ASSERT_COLOR, line, length, false);
}

/// Writes an assert variable message to the console.
void LoggerConsoleWritePolicy::WriteAssertVariable(const LoggerBase::Line& line, uint32_t length)
{
	CriticalSection::ScopedLock lock(cs_);
	
	WriteToConsole(console_, ASSERT_VARIABLE_COLOR, line, length, false);
}


X_NAMESPACE_END

