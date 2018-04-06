#pragma once

#ifndef X_CONSOLE_H
#define X_CONSOLE_H

#include "Math\XAlignment.h"
#include "Math\XPair.h"

// #include "Types/Rectangle.h"

X_NAMESPACE_BEGIN(core)

/// \ingroup Platform
/// \brief A class for manipulating Windows-specific consoles.
/// \remark Only one instance of this class should be created because Windows does not allow more than one console
/// to be created in a system process.
class Console
{
public:
    typedef Alignment Alignment;
    typedef Vec2i Position;
    typedef Recti Rect;

    /// \brief Constructs a console with the given title.
    /// \details Standard handles like stdout, stdin and stderr will be redirected to the console. Additionally, the
    /// console will try to set the icon with the resource ID 101.
    explicit Console(const wchar_t* title);

    /// Frees all resources.
    ~Console(void);

    /// Sets the console title.
    void SetTitle(const wchar_t* title);

    /// \brief Sets the console window size and number of lines stored internally, in character units.
    void SetSize(unsigned int windowWidth, unsigned int windowHeight, unsigned int numLines);

    /// Sets the cursor position inside the console, in character units.
    void SetCursorPosition(unsigned int x, unsigned int y);

    /// Moves the console window to a certain position, in pixel units.
    void MoveTo(int x, int y);

    /// Moves the console window to a certain position, in pixel units.
    void MoveTo(const Position& position);

    /// Aligns the console window to any xRect.
    void AlignTo(const Rect& xRect, AlignmentFlags alignment);

    /// Returns the console xRect, in pixel units.
    Rect GetRect(void) const;

    /// \brief Tries to read a key from the console, and returns its virtual key-code.
    /// \details Returns 0 if no key has been pressed.
    char ReadKey(void) const;
    /// blocks untill gets a key from console
    char ReadKeyBlocking(void) const;

    /// Shows/hides the console window.
    void Show(bool show);

    /// Redirects the std logging (output,error)
    void RedirectSTD(void);

    void PressToContinue(void) const;

    /// \brief Returns the native console object.
    /// \remark For internal use only.
    inline HANDLE GetNativeConsole(void) const;

    /// \brief Returns the native console window handle.
    /// \remark For internal use only.
    inline HWND GetNativeConsoleWindow(void) const;

private:
    X_NO_COPY(Console);
    X_NO_ASSIGN(Console);

    HANDLE console_;
    HANDLE consoleInput_;
    HWND window_;
    FILE* stdout_;
    FILE* stdin_;
    FILE* stderr_;
};

#include "Console.inl"

X_NAMESPACE_END

#endif
