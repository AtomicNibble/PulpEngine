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

class Window
{
    X_NO_COPY(Window);
    X_NO_ASSIGN(Window);

public:
    /// Types for position size etc.
    typedef AlignmentFlags AlignmentFlags;
    typedef Vec2i Position;
    typedef Recti Rect;

    struct Mode
    {
        enum Enum : uint32_t
        {
            NONE,
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

    Window();
    ~Window(void);

    static void RegisterVars(void);

    bool Create(const wchar_t* const pTitle, Rect r, Mode::Enum mode);
    void Destroy(void);
    void CustomFrame(bool enable);

    Notification::Enum PumpMessages(void);

    X_INLINE void Show(void);
    X_INLINE void Hide(void);
    X_INLINE void Close(void);
    X_INLINE void Minamise(void);   
    X_INLINE void Maximise(void);
    X_INLINE void Restore(void);
    X_INLINE void HideClientCursor(bool hide);
    X_INLINE void FixedAspectRatioSizing(bool enable);
    X_INLINE bool HasFocus(void) const;
    X_INLINE bool isMaximized(void) const;
    X_INLINE Mode::Enum getMode(void) const;
    //	X_INLINE bool isValid(void);
    
    void SetMode(Mode::Enum mode);

    void ClipCursorToWindow(void);
    Vec2i GetCusroPosClient(void);
    static Vec2i GetCusroPos(void);

    void MoveTo(int x, int y);
    void MoveTo(const Position& position);
    void AlignTo(const Rect& rect, AlignmentFlags alignment);
    void SetRect(const Rect& rect);

    void SetTitle(const char* str);

    bool Hasfocus(void) const;

    Rect GetRect(void) const;
    Rect GetClientRect(void) const;

public:
    X_INLINE const uint32_t GetNumMsgs(void) const;
    X_INLINE const uint32_t GetNumMsgsClear(void);

    X_INLINE uint32_t GetClientWidth(void) const;
    X_INLINE uint32_t GetClientHeight(void) const;

    // Returns the Rect of the monitor with the largest intersection with the window
    Rect GetActiveMonitorRect(void);

public:
    // Returns the Rect of the primary display monitor
    static Rect GetPrimaryRect(void);
    // Returns the Rect of the primary display monitor, not overlapping the taskbar.
    static Rect GetPrimaryRectExTaskBar(void);
    // Returns the Rect of the complete desktop area, spanning all monitors and overlapping the taskbar.
    static Rect GetDesktopRect(void);

    X_INLINE PLATFORM_HWND GetNativeWindow(void);
    X_INLINE const PLATFORM_HWND GetNativeWindow(void) const;

private:
    friend static LRESULT WndProcProxy(Window* pWindow, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void onSizing(WPARAM side, RECT* pRect);

private:
    X_INLINE static bool isDebugEnable(void);

protected:
    uint32_t numMsgs_;
    Mode::Enum mode_;
    PLATFORM_HWND window_;
    bool hideClientCursor_;
    bool sizingFixedAspectRatio_;
    bool hasFocus_;
    bool maximized_;
    bool close_;

    xFrame* pFrame_;

    static int32_t s_var_windowDebug;
};

#include "Window.inl"

X_NAMESPACE_END

#endif // _X_WINDOW_H_
