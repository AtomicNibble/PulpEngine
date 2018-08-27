#include "EngineCommon.h"

#include "Window.h"
#include "CustomFrame.h"

#include "Util\LastError.h"

#include <ICore.h>
#include <IConsole.h>

#include "resource.h"

X_NAMESPACE_BEGIN(core)

static X_INLINE LRESULT WndProcProxy(Window* pWindow, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return pWindow->WndProc(hWnd, msg, wParam, lParam);
}

namespace
{
    const wchar_t* g_ClassName = X_ENGINE_NAME_W L"Engine";

    bool g_ClassRegisterd = false;

    LRESULT CALLBACK PreWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void RegisterClass(void)
    {
        if (!g_ClassRegisterd) {

            auto hInstance = GetModuleHandle(NULL);
            WNDCLASSEXW wcex;

            if (GetClassInfoExW(hInstance, g_ClassName, &wcex)) {
                return;
            }

            wcex.cbSize = sizeof(wcex);
            wcex.style = /*CS_HREDRAW | CS_VREDRAW |*/ CS_DBLCLKS | CS_OWNDC;
            wcex.lpfnWndProc = PreWndProc;
            wcex.cbClsExtra = 0;
            wcex.cbWndExtra = 0;
            wcex.hInstance = hInstance;
            wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ENGINE_LOGO));
            wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
            wcex.hbrBackground = 0;
            wcex.lpszMenuName = 0;
            wcex.lpszClassName = g_ClassName;
            wcex.hIconSm = 0;

            if (RegisterClassExW(&wcex) == 0) {
                core::lastError::Description Dsc;
                X_ERROR("Window", "Failed to register class. Err: %s", core::lastError::ToString(Dsc));
                return;
            }

            g_ClassRegisterd = true;
        }
    }

    // We have a pre and final winproc.
    // The pre one waits untill we can get the class pointer.
    // Then switches to a lightweight winproc as each window is only created once.
    // and we no longer need to do checks for msgs.

    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        LONG_PTR data = GetWindowLongPtr(hWnd, GWLP_USERDATA); // GetWindowLongPtr required for 64
        auto* pClass = reinterpret_cast<Window*>(data);

        return WndProcProxy(pClass, hWnd, msg, wParam, lParam);
    }

    LRESULT CALLBACK PreWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE) {
            LPCREATESTRUCT pInfo = reinterpret_cast<LPCREATESTRUCT>(lParam);
            Window* pClass = reinterpret_cast<Window*>(pInfo->lpCreateParams);

            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pClass));
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));

            return WndProcProxy(pClass, hWnd, msg, wParam, lParam);
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    Recti Convert(const RECT& r)
    {
        return Recti(r.left, r.top, r.right, r.bottom);
    }

} // namespace

int32_t Window::s_var_windowDebug = 0;

/// --------------------------------------------------------------------------------------------------------

Window::Window() :
    numMsgs_(0),
    mode_(Mode::NONE),
    hideClientCursor_(false),
    sizingFixedAspectRatio_(true),
    hasFocus_(false),
    maximized_(false),
    close_(false),
    pFrame_(nullptr)
{
    RegisterClass();
}

Window::~Window(void)
{
    Destroy();
    X_DELETE(pFrame_, gEnv->pArena);
}

/// --------------------------------------------------------------------------------------------------------

void Window::RegisterVars(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pConsole);

    ADD_CVAR_REF("win_debug", s_var_windowDebug, 0, 0, 1, core::VarFlag::SYSTEM,
        "Window debug. 0=disable, 1=enable.");
}

/// --------------------------------------------------------------------------------------------------------

bool Window::Create(const wchar_t* const pTitle, Rect r, Mode::Enum mode)
{
    lastError::Description Dsc;

    if (!g_ClassRegisterd) {
        X_FATAL("Window", "Class not registered.");
        return false;
    }

    mode_ = mode;

    // Make the size the client size.
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = r.getWidth();
    rect.bottom = r.getHeight();

    if (!::AdjustWindowRect(&rect, mode, FALSE)) {
        X_ERROR("Window", "Failed to adjust rect. Err: %s", core::lastError::ToString(Dsc));
        return false;
    }

    r.x2 = r.x1 + (rect.right - rect.left);
    r.y2 = r.y1 + (rect.bottom - rect.top);

    window_ = ::CreateWindowExW(
        0,
        g_ClassName,
        pTitle,
        mode,
        r.x1, r.y1, 
        r.getWidth(), r.getHeight(),
        GetDesktopWindow(),
        0,
        GetModuleHandle(NULL),
        this // required for winproc
    );

#if 0
    HICON hIcon = NULL; //  LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ENGINE_LOGO));
    SendMessage(window_, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SendMessage(window_, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
#endif

    X_ERROR_IF(window_ == NULL, "Window", "Failed to create window. Title: '%ls'. Error: %s", pTitle, lastError::ToString(Dsc));

    return window_ != NULL;
}

void Window::Destroy(void)
{
    ::DestroyWindow(window_);
    window_ = NULL;

    mode_ = Mode::NONE;
    hasFocus_ = false;
}

void Window::CustomFrame(bool enable)
{
    if (enable) {
        if (pFrame_ == nullptr) {
            pFrame_ = X_NEW(xFrame, gEnv->pArena, "Win32CustomFrame");

            ::RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            ::RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_NOCHILDREN | RDW_VALIDATE);
        }
        else {
            ::RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            ::RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_NOCHILDREN | RDW_VALIDATE);
        }
    }
    else {
        if (pFrame_ != nullptr) {
            X_DELETE_AND_NULL(pFrame_, gEnv->pArena);

            ::RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            ::RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_NOCHILDREN | RDW_VALIDATE);
        }
    }
}

Window::Notification::Enum Window::PumpMessages(void)
{
    MSG msg;
    uint32_t msgNumStart = numMsgs_;

    // need two for WM_INPUT goat shiz.
    while (::PeekMessage(&msg, window_, 0, WM_INPUT - 1, PM_REMOVE)) {
        numMsgs_++;

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    while (::PeekMessage(&msg, window_, WM_INPUT + 1, 0xffffffff, PM_REMOVE)) {
        numMsgs_++;

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    if (isDebugEnable()) {
        uint32_t pumpDelta = numMsgs_ - msgNumStart;
        X_LOG0("Window", "num messgaes pumped: %i total: %i", pumpDelta, numMsgs_);
    }

    if (close_) {
        return Notification::CLOSE;
    }

    return Notification::NONE;
}

/// --------------------------------------------------------------------------------------------------------

void Window::SetMode(Mode::Enum mode)
{
    if (mode_ == mode) {
        return;
    }

    DWORD dwStyle = GetWindowLong(window_, GWL_STYLE);

    dwStyle = dwStyle & ~mode_;

    SetWindowLong(window_, GWL_STYLE, dwStyle | mode);
    // TODO: pass HWND_NOTOPMOST for none fullscreen?
    SetWindowPos(window_, HWND_TOP, 0,0,0,0, SWP_NOSIZE | SWP_FRAMECHANGED);

    mode_ = mode;
}

void Window::ClipCursorToWindow(void)
{
    RECT r;
    ::GetClientRect(window_, &r);
    ::ClientToScreen(window_, reinterpret_cast<LPPOINT>(&r.left));
    ::ClientToScreen(window_, reinterpret_cast<LPPOINT>(&r.right));
    ::ClipCursor(&r);
}

Vec2i Window::GetCusroPosClient(void)
{
    POINT pos;

    {
        auto p = GetCusroPos();
        pos.x = p.x;
        pos.y = p.y;
    }

    if (!::ScreenToClient(window_, &pos)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to map cursor point to screen. Err: %s", core::lastError::ToString(Dsc));
    }

    return Vec2i(pos.x, pos.y);
}

Vec2i Window::GetCusroPos(void)
{
    POINT pos;
    if (!::GetCursorPos(&pos)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to GetCusroPos. Err: %s", core::lastError::ToString(Dsc));
    }

    return Vec2i(pos.x, pos.y);
}

void Window::MoveTo(int x, int y)
{
    RECT rect;
    if (!::GetWindowRect(window_, &rect)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to get rect. Err: %s", core::lastError::ToString(Dsc));
    }
    if (!::MoveWindow(window_, x, y, rect.right - rect.left, rect.bottom - rect.top, FALSE)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to move window. Err: %s", core::lastError::ToString(Dsc));
    }
}

void Window::MoveTo(const Position& position)
{
    MoveTo(position.x, position.y);
}

void Window::AlignTo(const Rect& Rect, AlignmentFlags alignment)
{
    Recti rect = GetRect();
    rect.Align(Rect, alignment);

    MoveTo(rect.x1, rect.y1);
}

void Window::SetRect(const Rect& rect)
{
    RECT r;
    r.left = 0;
    r.top = 0;
    r.right = rect.getWidth();
    r.bottom = rect.getHeight();

    if (!::AdjustWindowRect(&r, mode_, FALSE)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to adjust rect. Err: %s", core::lastError::ToString(Dsc));
    }

    Rect newRect;
    newRect.set(
        rect.x1,
        rect.y1,    
        rect.x1 + (r.right - r.left),
        rect.y1 + (r.bottom - r.top)
    );

    if (!::SetWindowPos(window_, nullptr, newRect.x1, newRect.y1, newRect.getWidth(), newRect.getHeight(), SWP_NOZORDER | SWP_NOREDRAW)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to set window rect. Err: %s", core::lastError::ToString(Dsc));
    }
}

Recti Window::GetRect(void) const
{
    RECT rect;
    if (!::GetWindowRect(window_, &rect)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to get rect. Err: %s", core::lastError::ToString(Dsc));
    }

    return Convert(rect);
}

Recti Window::GetClientRect(void) const
{
    RECT rect;
    if (!::GetClientRect(window_, &rect)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to get client rect. Err: %s", core::lastError::ToString(Dsc));
    }

    return Convert(rect);
}

/// --------------------------------------------------------------------------------------------------------


Recti Window::GetActiveMonitorRect(void)
{
    auto hMon = MonitorFromWindow(window_, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFO mi = { sizeof(mi) };
    if (!::GetMonitorInfo(hMon, &mi)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to get monitor rect. Err: %s", core::lastError::ToString(Dsc));
    }

    return Recti(
        mi.rcMonitor.left, 
        mi.rcMonitor.top,
        mi.rcMonitor.right,
        mi.rcMonitor.bottom);
}


// Returns the xRect of the primary display monitor, not overlapping the taskbar.
Recti Window::GetPrimaryRect(void)
{
    return Recti(
        0,
        0,
        ::GetSystemMetrics(SM_CXSCREEN),
        ::GetSystemMetrics(SM_CYSCREEN));
}

// Returns the Rect of the primary display monitor, not overlapping the taskbar.
Recti Window::GetPrimaryRectExTaskBar(void)
{
    RECT rect;
    ::SystemParametersInfo(SPI_GETWORKAREA, NULL, &rect, NULL);

    return Convert(rect);
}

// Returns the Rect of the complete desktop area, spanning all monitors and overlapping the taskbar.
Recti Window::GetDesktopRect(void)
{
    return Recti(
        ::GetSystemMetrics(SM_XVIRTUALSCREEN),
        ::GetSystemMetrics(SM_YVIRTUALSCREEN),
        ::GetSystemMetrics(SM_CXVIRTUALSCREEN),
        ::GetSystemMetrics(SM_CYVIRTUALSCREEN));
}


/// --------------------------------------------------------------------------------------------------------

LRESULT Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_SETCURSOR:
        {
            if (LOWORD(lParam) == HTCLIENT && hideClientCursor_) {
                ::SetCursor(NULL);
                return TRUE;
            }
            break;
        }
        case WM_ACTIVATE:
        {
            CoreEventData ed;
            ed.event = CoreEvent::CHANGE_FOCUS;
            ed.focus.active = (wParam != WA_INACTIVE);
            
            gEnv->pCore->GetCoreEventDispatcher()->QueueCoreEvent(ed);
            hasFocus_ = ed.focus.active > 0;
            break;
        }
        case WM_CLOSE:
        {
            close_ = true;
            return 0;
        }
        case WM_SIZING:
        {
            if (sizingFixedAspectRatio_) {
                onSizing(wParam, reinterpret_cast<RECT*>(lParam));
                return true;
            }
            break;
        }
        case WM_SIZE:
        {
            CoreEventData ed;
            ed.event = CoreEvent::RESIZE;
            ed.resize.width = static_cast<int16_t>(LOWORD(lParam));
            ed.resize.height = static_cast<int16_t>(HIWORD(lParam));

            maximized_ = (wParam == SIZE_MAXIMIZED);

            gEnv->pCore->GetCoreEventDispatcher()->QueueCoreEvent(ed);
            break;
        }
        case WM_MOVE:
        {
            CoreEventData ed;
            ed.event = CoreEvent::MOVE;
            ed.move.clientX = static_cast<int16_t>(LOWORD(lParam));
            ed.move.clientY = static_cast<int16_t>(HIWORD(lParam));

            RECT r;
            r.left = 0;
            r.top = 0;
            r.right = 1;
            r.bottom = 1;

            if (!::AdjustWindowRect(&r, mode_, FALSE)) {
                core::lastError::Description Dsc;
                X_ERROR("Window", "Failed to adjust rect. Err: %s", core::lastError::ToString(Dsc));
            }

            ed.move.windowX = safe_static_cast<int16_t>(ed.move.clientX + r.left);
            ed.move.windowY = safe_static_cast<int16_t>(ed.move.clientY + r.top);

            gEnv->pCore->GetCoreEventDispatcher()->QueueCoreEvent(ed);
            break;
        }
    }

    if (pFrame_) {
        return pFrame_->DrawFrame(hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/// --------------------------------------------------------------------------------------------------------


void Window::onSizing(WPARAM side, RECT* pRect)
{
    // restrict to a standard aspect ratio
    int width = pRect->right - pRect->left;
    int height = pRect->bottom - pRect->top;

    // Adjust width/height for window decoration
    RECT decoRect = { 0, 0, 0, 0 };
    ::AdjustWindowRect(&decoRect, mode_ | WS_SYSMENU, FALSE);
    int decoWidth = decoRect.right - decoRect.left;
    int decoHeight = decoRect.bottom - decoRect.top;

    width -= decoWidth;
    height -= decoHeight;

    const int SCREEN_WIDTH = 640;
    const int SCREEN_HEIGHT = 480;

    // Clamp to a minimum size
    if (width < SCREEN_WIDTH / 4) {
        width = SCREEN_WIDTH / 4;
    }
    if (height < SCREEN_HEIGHT / 4) {
        height = SCREEN_HEIGHT / 4;
    }

    const int minWidth = height * 4 / 3;
    const int maxHeight = width * 3 / 4;

    const int maxWidth = height * 16 / 9;
    const int minHeight = width * 9 / 16;

    // Set the new size
    switch (side) {
        case WMSZ_LEFT:
            pRect->left = pRect->right - width - decoWidth;
            pRect->bottom = pRect->top + math<int>::clamp(height, minHeight, maxHeight) + decoHeight;
            break;
        case WMSZ_RIGHT:
            pRect->right = pRect->left + width + decoWidth;
            pRect->bottom = pRect->top + math<int>::clamp(height, minHeight, maxHeight) + decoHeight;
            break;
        case WMSZ_BOTTOM:
        case WMSZ_BOTTOMRIGHT:
            pRect->bottom = pRect->top + height + decoHeight;
            pRect->right = pRect->left + math<int>::clamp(width, minWidth, maxWidth) + decoWidth;
            break;
        case WMSZ_TOP:
        case WMSZ_TOPRIGHT:
            pRect->top = pRect->bottom - height - decoHeight;
            pRect->right = pRect->left + math<int>::clamp(width, minWidth, maxWidth) + decoWidth;
            break;
        case WMSZ_BOTTOMLEFT:
            pRect->bottom = pRect->top + height + decoHeight;
            pRect->left = pRect->right - math<int>::clamp(width, minWidth, maxWidth) - decoWidth;
            break;
        case WMSZ_TOPLEFT:
            pRect->top = pRect->bottom - height - decoHeight;
            pRect->left = pRect->right - math<int>::clamp(width, minWidth, maxWidth) - decoWidth;
            break;
        default:
            // sometimes I get a 0x9?
            break;
    }
}


X_NAMESPACE_END