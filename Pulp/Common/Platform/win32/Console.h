#pragma once

#ifndef X_CONSOLE_H
#define X_CONSOLE_H

#include "Math\XAlignment.h"
#include "Math\XPair.h"

X_NAMESPACE_BEGIN(core)

class Console
{
public:
    typedef Alignment Alignment;
    typedef Vec2i Position;
    typedef Recti Rect;

    explicit Console(const wchar_t* title);

    ~Console(void);

    void setTitle(const wchar_t* title);
    void setSize(uint32_t windowWidth, uint32_t windowHeight, uint32_t numLines);
    void setCursorPosition(uint32_t x, uint32_t y);
    void moveTo(int x, int y);
    void moveTo(const Position& position);

    void alignTo(const Rect& xRect, AlignmentFlags alignment);

    Rect getRect(void) const;

    char readKey(void) const;
    char readKeyBlocking(void) const;

    void show(bool show);

    void redirectSTD(void);
    void pressToContinue(void) const;

    inline HANDLE getNativeConsole(void) const;
    inline PLATFORM_HWND getNativeConsoleWindow(void) const;

private:
    X_NO_COPY(Console);
    X_NO_ASSIGN(Console);

    HANDLE console_;
    HANDLE consoleInput_;
    PLATFORM_HWND window_;
    FILE* stdout_;
    FILE* stdin_;
    FILE* stderr_;
};

#include "Console.inl"

X_NAMESPACE_END

#endif
