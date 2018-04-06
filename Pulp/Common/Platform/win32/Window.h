#pragma once

#ifndef _X_WINDOW_H_
#define _X_WINDOW_H_

#ifdef CreateWindow
#undef CreateWindow
#endif

#ifdef RegisterClass
#undef RegisterClass
#endif

X_NAMESPACE_BEGIN(core)

class xFrame;

// this should be a nice little wrapper for windows that allows resizing and all that shit.
// Styles etc :D

class xWindow
{
    X_NO_COPY(xWindow);
    X_NO_ASSIGN(xWindow);

public:
    /// Types for position size etc.
    typedef AlignmentFlags AlignmentFlags;
    typedef Vec2i Position;
    typedef Recti Rect;

    struct Mode
    {
        enum Enum : uint32_t
        {
            FULLSCREEN = WS_POPUP,                            ///< Fullscreen window.
            APPLICATION = WS_OVERLAPPEDWINDOW,                ///< Application window (windowed mode with standard icons)
            TOOL = WS_THICKFRAME | WS_CAPTION | WS_OVERLAPPED ///< Tool window (windowed mode with no icons)
        };
    };

    struct Notification
    {
        enum Enum
        {
            CLOSE, ///< The window is about to be closed.
            NONE
        };
    };

    xWindow();
    ~xWindow(void);

    static void RegisterVars(void);

    bool Create(const wchar_t* const Title, int x, int y, int width, int height, Mode::Enum mode);

    Notification::Enum PumpMessages(void);

    void CustomFrame(bool val);

    X_INLINE void Show(void);
    X_INLINE void Hide(void);
    X_INLINE void Close(void);
    X_INLINE void Minamise(void);
    X_INLINE void MaxiMise(void);
    X_INLINE void Restore(void);
    X_INLINE void Destroy(void);
    X_INLINE void HideClientCursor(bool hide = true);
    //	X_INLINE bool isValid(void);

    void ClipCursorToWindow(void);

    virtual void MoveTo(int x, int y);
    virtual void MoveTo(const Position& position);
    virtual void AlignTo(const Rect& Rect, AlignmentFlags alignment);

    void SetTitle(const char* str);

    bool Hasfocus(void) const;

    Rect GetRect(void) const;
    Rect GetClientRect(void) const;

public:
    X_INLINE const uint32_t GetNumMsgs(void) const;
    X_INLINE const uint32_t GetNumMsgsClear(void);

    X_INLINE uint32_t GetClientWidth(void) const;
    X_INLINE uint32_t GetClientHeight(void) const;

public:
    // Returns the xRect of the primary display monitor, not overlapping the taskbar.
    static Rect GetPrimaryRect(void);
    // Returns the xRect of the complete desktop area, spanning all monitors and overlapping the taskbar.
    static Rect GetDesktopRect(void);

    X_INLINE PLATFORM_HWND GetNativeWindow(void);
    X_INLINE const PLATFORM_HWND GetNativeWindow(void) const;

    virtual LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    X_INLINE static bool isDebugEnable(void);

protected:
    uint32_t numMsgs_;
    PLATFORM_HWND window_;
    bool hideClientCursor_;

    xFrame* pFrame_;

    static int32_t s_var_windowDebug;
};

#include "Window.inl"

// extern xWindow* g_gamewindow;

X_NAMESPACE_END

#endif // _X_WINDOW_H_
