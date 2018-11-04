#include "EngineCommon.h"

#include "Platform\Console.h"
#include "LoggerConsoleWritePolicy.h"

#include "Util\LastError.h"

#include <ICore.h>

X_NAMESPACE_BEGIN(core)

namespace
{
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
        COL_LIGHTPINK,
        COL_PINK
    };

    X_INLINE unsigned __int16 MakeColor(Cols For, Cols Back)
    {
        return static_cast<uint16_t>(For + (Back << 4));
    }

    const unsigned int COLOR_TABLE[16] = {
        RGB(0x10, 0x10, 0x10),
        RGB(0xff, 0xff, 0xff),
        RGB(0x10, 0x10, 0x10),
        RGB(0x70, 0x70, 0x70),

        RGB(0, 0x66, 0xcc),
        RGB(0x56, 0xa8, 0x3c),
        RGB(0xff, 0, 0),
        RGB(0xff, 0xff, 0x1c),
        RGB(0xa3, 0x38, 0xff),
        RGB(0, 0xe0, 0xe0),

        RGB(0, 0x66, 0xcc),
        RGB(0, 0x66, 0),
        RGB(0xcc, 0, 0),

        RGB(0xcc, 0x70, 0),
        RGB(0xff, 0xaa, 0x9b),
        RGB(0xff, 0x69, 0xc8),
    };

    unsigned __int16 COLOR_CODE_TABLE[10] = {
        MakeColor(COL_BLACK, COL_GRAY),
        MakeColor(COL_RED, COL_GRAY),
        MakeColor(COL_YELLOW, COL_GRAY),
        MakeColor(COL_BLUE, COL_GRAY),
        MakeColor(COL_BLUE_LIGHT, COL_GRAY),
        MakeColor(COL_LIGHTPINK, COL_GRAY),
        MakeColor(COL_ORANGE, COL_GRAY),
        MakeColor(COL_WHITE, COL_GRAY),
        MakeColor(COL_GREEN, COL_GRAY),
        MakeColor(COL_PINK, COL_GRAY),
    };

} // namespace

LoggerConsoleWritePolicy::LoggerConsoleWritePolicy(const Console& console) :
    console_(console.getNativeConsole()),
    cs_(50)
{
}

void LoggerConsoleWritePolicy::Init(void)
{
    CONSOLE_SCREEN_BUFFER_INFOEX InfoEx;

    InfoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

    lastError::Description Dsc;

    if (!GetConsoleScreenBufferInfoEx(console_, &InfoEx)) {
        X_ERROR("ConsoleLogger", "Failed get buffer info. Error: %s", lastError::ToString(Dsc));
    }

    memcpy(InfoEx.ColorTable, COLOR_TABLE, sizeof(COLOR_TABLE));

    ++InfoEx.srWindow.Right;
    ++InfoEx.srWindow.Bottom;

    if (!SetConsoleScreenBufferInfoEx(console_, &InfoEx)) {
        X_ERROR("ConsoleLogger", "Failed to set buffer info. Error: %s", lastError::ToString(Dsc));
    }
}

void LoggerConsoleWritePolicy::Exit(void)
{
}

#include <wchar.h>

namespace
{
    const unsigned __int16 LOG_COLOR = MakeColor(COL_WHITE, COL_GRAY); // COL_WHITE;
    const unsigned __int16 WARNING_COLOR = MakeColor(COL_WHITE, COL_ORANGE);
    const unsigned __int16 ERROR_COLOR = MakeColor(COL_RED, COL_GRAY);
    const unsigned __int16 FATAL_ERROR_COLOR = MakeColor(COL_GRAY, COL_RED);
    const unsigned __int16 ASSERT_COLOR = MakeColor(COL_RED, COL_BLACK);
    const unsigned __int16 ASSERT_VARIABLE_COLOR = MakeColor(COL_WHITE, COL_RED);
    const unsigned __int16 CHANNEL_COLOR = MakeColor(COL_BLUE_LIGHT, COL_GRAY);
    const unsigned __int16 STRING_COLOR = MakeColor(COL_GREEN, COL_BLACK); // COL_GREEN;

    void WriteToConsole(HANDLE console, unsigned __int16 color, const char* asciiMsg,
        unsigned int length, bool colorizeExtended)
    {
        wchar_t wMsg[sizeof(LoggerBase::Line)];

        DWORD NumberOfCharsWritten;
        DWORD SectionSize = 0;
        bool isString = false;
        bool hasCoolorCode = false;
        bool ColorSet = false;
        bool channelEnded = false;
        const wchar_t *pCur, *pMsgEnd;

        if (length < 1) {
            return;
        }

        // CharToOemA( asciiMsg, oemMsg );
        strUtil::Convert(asciiMsg, wMsg);

        // has a ^?
        if (core::strUtil::Find(wMsg, wMsg + length, L'^')) {
            hasCoolorCode = true;
        }

        pCur = wMsg;
        pMsgEnd = wMsg + length;

        do {
            if (SectionSize >= length) {
                break;
            }

            if (!channelEnded && *(pCur + SectionSize) == L'|') {
                channelEnded = true;

                SetConsoleTextAttribute(console, CHANNEL_COLOR);

                NumberOfCharsWritten = 0;
                WriteConsoleW(console, pCur, SectionSize, &NumberOfCharsWritten, 0);

                SetConsoleTextAttribute(console, color);

                length -= SectionSize;
                pCur = pCur + SectionSize;
                SectionSize = 0;
            }

            if (colorizeExtended) {
                wchar_t curChar = *(pCur + SectionSize);
                if (hasCoolorCode && curChar == L'^' && SectionSize < length) {
                    wchar_t colChar = *(pCur + SectionSize + 1);
                    if (core::strUtil::IsDigitW(colChar)) {
                        int colorIndex = colChar - L'0';

                        // we draw anything then change color.
                        NumberOfCharsWritten = 0;
                        WriteConsoleW(console, pCur, SectionSize, &NumberOfCharsWritten, 0);

                        SetConsoleTextAttribute(console, COLOR_CODE_TABLE[colorIndex]);

                        ColorSet = true;

                        SectionSize += 2;

                        length -= SectionSize;
                        pCur = pCur + SectionSize;
                        SectionSize = 0;

                        if (length > 0 && *pCur == L'\"') {
                            isString = (isString == 0);
                        }
                    }
                }
                else if (curChar == L'\"') {
                    if (isString) {
                        ++SectionSize;
                    }

                    NumberOfCharsWritten = 0;
                    WriteConsoleW(console, pCur, SectionSize, &NumberOfCharsWritten, 0);

                    // still make text green.
                    if (!isString) {
                        SetConsoleTextAttribute(console, STRING_COLOR);
                    }
                    else {
                        SetConsoleTextAttribute(console, color);
                    }

                    length -= SectionSize;
                    pCur = pCur + SectionSize;
                    SectionSize = 0;

                    isString = (isString == 0);
                    if (!isString) {
                        continue; // we need to check for color code right after a closing "
                    }
                }
            }
            else {
                wchar_t curChar = *(pCur + SectionSize);

                // skip color codes.
                if (hasCoolorCode && curChar == L'^' && SectionSize < length) {
                    wchar_t colChar = *(pCur + SectionSize + 1);
                    if (core::strUtil::IsDigitW(colChar)) {
                        // we draw anything then change color.
                        NumberOfCharsWritten = 0;
                        WriteConsoleW(console, pCur, SectionSize, &NumberOfCharsWritten, 0);

                        SectionSize += 2;

                        length -= SectionSize;
                        pCur = pCur + SectionSize;
                        SectionSize = 0;
                    }
                }
            }

            ++SectionSize;
        } while (pCur < pMsgEnd);

        if (!ColorSet) {
            SetConsoleTextAttribute(console, color);
        }

        DWORD zeroEnd = 0;
        WriteConsoleW(console, pCur, length, &zeroEnd, 0);

        // reset
        SetConsoleTextAttribute(console, LOG_COLOR);
    }

}; // namespace

void LoggerConsoleWritePolicy::WriteLog(const LoggerBase::Line& line, uint32_t length)
{
    CriticalSection::ScopedLock lock(cs_);

    WriteToConsole(console_, LOG_COLOR, line, length, true);
}

void LoggerConsoleWritePolicy::WriteWarning(const LoggerBase::Line& line, uint32_t length)
{
    CriticalSection::ScopedLock lock(cs_);

    WriteToConsole(console_, WARNING_COLOR, line, length, false);
}

void LoggerConsoleWritePolicy::WriteError(const LoggerBase::Line& line, uint32_t length)
{
    CriticalSection::ScopedLock lock(cs_);

    WriteToConsole(console_, ERROR_COLOR, line, length, false);
}

void LoggerConsoleWritePolicy::WriteFatal(const LoggerBase::Line& line, uint32_t length)
{
    CriticalSection::ScopedLock lock(cs_);

    WriteToConsole(console_, FATAL_ERROR_COLOR, line, length, false);
}

void LoggerConsoleWritePolicy::WriteAssert(const LoggerBase::Line& line, uint32_t length)
{
    CriticalSection::ScopedLock lock(cs_);

    WriteToConsole(console_, ASSERT_COLOR, line, length, false);
}

void LoggerConsoleWritePolicy::WriteAssertVariable(const LoggerBase::Line& line, uint32_t length)
{
    CriticalSection::ScopedLock lock(cs_);

    WriteToConsole(console_, ASSERT_VARIABLE_COLOR, line, length, false);
}

X_NAMESPACE_END
