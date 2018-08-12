#include "EngineCommon.h"

#include "Window.h"
#include "CustomFrame.h"

#include "Util\LastError.h"

#include <ICore.h>
#include <IConsole.h>

#include "resource.h"

X_NAMESPACE_BEGIN(core)

static X_INLINE LRESULT WndProcProxy(xWindow* pWindow, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

    void UnRegisterClass(void)
    {
        // I can't really safley unregister this.
        // since if another dll in the process registeres it
        // i can't know when it will unregister it.

#if 0
		if (g_ClassRegisterd)
		{
			g_ClassRegisterd = false;

			wchar_t classNameWide[256] = { 0 };
			core::strUtil::Convert(g_ClassName, classNameWide);

			if (!UnregisterClassW(classNameWide, GetModuleHandle(NULL)))
			{

			}
		}
#endif
    }

    // We have a pre and final winproc.
    // The pre one waits untill we can get the class pointer.
    // Then switches to a lightweight winproc as each window is only created once.
    // and we no longer need to do checks for msgs.

    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        LONG_PTR data = GetWindowLongPtr(hWnd, GWLP_USERDATA); // GetWindowLongPtr required for 64
        auto* pClass = reinterpret_cast<xWindow*>(data);

        return WndProcProxy(pClass, hWnd, msg, wParam, lParam);
    }

    LRESULT CALLBACK PreWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE) {
            LPCREATESTRUCT pInfo = reinterpret_cast<LPCREATESTRUCT>(lParam);
            xWindow* pClass = reinterpret_cast<xWindow*>(pInfo->lpCreateParams);

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

int32_t xWindow::s_var_windowDebug = 0;

/// --------------------------------------------------------------------------------------------------------

xWindow::xWindow() :
    numMsgs_(0),
    mode_(Mode::NONE),
    hideClientCursor_(false),
    sizingFixedAspectRatio_(true),
    pFrame_(nullptr)
{
    RegisterClass();
}

xWindow::~xWindow(void)
{
    Destroy();
    UnRegisterClass();
    X_DELETE(pFrame_, gEnv->pArena);
}

/// --------------------------------------------------------------------------------------------------------

void xWindow::RegisterVars(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pConsole);

    ADD_CVAR_REF("win_debug", s_var_windowDebug, 0, 0, 1, core::VarFlag::SYSTEM,
        "Window debug. 0=disable, 1=enable.");
}

/// --------------------------------------------------------------------------------------------------------

bool xWindow::Create(const wchar_t* const pTitle, int x, int y, int width, int height, Mode::Enum mode)
{
    lastError::Description Dsc;

    if (!g_ClassRegisterd) {
        X_FATAL("Window", "Class not registered.");
        return false;
    }

    mode_ = mode;

    if (mode != xWindow::Mode::FULLSCREEN) {
        // Make the size the client size.
        RECT Rect;
        Rect.left = Rect.top = 0;
        Rect.right = width;
        Rect.bottom = height;

        ::AdjustWindowRect(&Rect, mode, FALSE);

        width = safe_static_cast<int, LONG>(Rect.right - Rect.left);
        height = safe_static_cast<int, LONG>(Rect.bottom - Rect.top);
    }

    window_ = CreateWindowExW(
        0,
        g_ClassName,
        pTitle,
        mode,
        x, y,
        width, height,
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

void xWindow::CustomFrame(bool enable)
{
    if (enable) {
        if (pFrame_ == nullptr) {
            pFrame_ = X_NEW(xFrame, gEnv->pArena, "Win32CustomFrame");

            RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_NOCHILDREN | RDW_VALIDATE);
        }
        else {
            RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_NOCHILDREN | RDW_VALIDATE);
        }
    }
    else {
        if (pFrame_ != nullptr) {
            X_DELETE_AND_NULL(pFrame_, gEnv->pArena);

            RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            RedrawWindow(window_, NULL, NULL, RDW_FRAME | RDW_NOCHILDREN | RDW_VALIDATE);
        }
    }
}

xWindow::Notification::Enum xWindow::PumpMessages(void)
{
    MSG msg;
    uint32_t msgNumStart = numMsgs_;

    // need two for WM_INPUT goat shiz.
    while (PeekMessage(&msg, this->window_, 0, WM_INPUT - 1, PM_REMOVE)) {
        numMsgs_++;

        if (msg.message == WM_CLOSE) {
            return Notification::CLOSE;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    while (PeekMessage(&msg, this->window_, WM_INPUT + 1, 0xffffffff, PM_REMOVE)) {
        numMsgs_++;

        if (msg.message == WM_CLOSE) {
            return Notification::CLOSE;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (isDebugEnable()) {
        uint32_t pumpDelta = numMsgs_ - msgNumStart;
        X_LOG0("Window", "num messgaes pumped: %i total: %i", pumpDelta, numMsgs_);
    }
    return Notification::NONE;
}

/// --------------------------------------------------------------------------------------------------------


void xWindow::ClipCursorToWindow(void)
{
    RECT r;
    ::GetClientRect(window_, &r);
    ClientToScreen(window_, reinterpret_cast<LPPOINT>(&r.left));
    ClientToScreen(window_, reinterpret_cast<LPPOINT>(&r.right));
    ClipCursor(&r);
}

Vec2i xWindow::GetCusroPosClient(void)
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

Vec2i xWindow::GetCusroPos(void)
{
    POINT pos;
    if (!::GetCursorPos(&pos)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to GetCusroPos. Err: %s", core::lastError::ToString(Dsc));
    }

    return Vec2i(pos.x, pos.y);
}

void xWindow::MoveTo(int x, int y)
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

void xWindow::MoveTo(const Position& position)
{
    MoveTo(position.x, position.y);
}

void xWindow::AlignTo(const Rect& Rect, AlignmentFlags alignment)
{
    Recti rect = GetRect();
    rect.Align(Rect, alignment);

    MoveTo(rect.x1, rect.y1);
}

Recti xWindow::GetRect(void) const
{
    RECT rect;
    if (!::GetWindowRect(window_, &rect)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to get rect. Err: %s", core::lastError::ToString(Dsc));
    }

    return Convert(rect);
}

Recti xWindow::GetClientRect(void) const
{
    RECT rect;
    if (!::GetClientRect(window_, &rect)) {
        core::lastError::Description Dsc;
        X_ERROR("Window", "Failed to get client rect. Err: %s", core::lastError::ToString(Dsc));
    }

    return Convert(rect);
}

/// --------------------------------------------------------------------------------------------------------

// Returns the xRect of the primary display monitor, not overlapping the taskbar.
Recti xWindow::GetPrimaryRect(void)
{
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, NULL, &rect, NULL);

    return Convert(rect);
}

// Returns the xRect of the complete desktop area, spanning all monitors and overlapping the taskbar.
Recti xWindow::GetDesktopRect(void)
{
    return Recti(
        GetSystemMetrics(SM_XVIRTUALSCREEN),
        GetSystemMetrics(SM_YVIRTUALSCREEN),
        GetSystemMetrics(SM_CXVIRTUALSCREEN),
        GetSystemMetrics(SM_CYVIRTUALSCREEN));
}


/// --------------------------------------------------------------------------------------------------------

LRESULT xWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_SETCURSOR:
        {
            if (LOWORD(lParam) == HTCLIENT && hideClientCursor_) {
                SetCursor(NULL);
                return TRUE;
            }
            break;
        }
        case WM_ACTIVATE:
        {
            int32_t active = (wParam != WA_INACTIVE);
            gEnv->pCore->GetCoreEventDispatcher()->OnCoreEvent(CoreEvent::CHANGE_FOCUS, active, lParam);
            break;
        }
        case WM_SIZE:
        {
            gEnv->pCore->GetCoreEventDispatcher()->OnCoreEvent(CoreEvent::RESIZE, wParam, lParam);
            break;
        }
        case WM_SIZING:
        {
            if (sizingFixedAspectRatio_) {
                onSizing(wParam, reinterpret_cast<RECT*>(lParam));
                return true;
            }
            break;
        }
        case WM_MOVE:
        {
            int32_t xPos = static_cast<short>(LOWORD(lParam));
            int32_t yPos = static_cast<short>(HIWORD(lParam));

            gEnv->pCore->GetCoreEventDispatcher()->OnCoreEvent(CoreEvent::MOVE, xPos, yPos);
            break;
        }
    }

    if (pFrame_) {
        return pFrame_->DrawFrame(hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/// --------------------------------------------------------------------------------------------------------


void xWindow::onSizing(WPARAM side, RECT* pRect)
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
            X_ASSERT_NOT_IMPLEMENTED();
            break;
    }
}


X_NAMESPACE_END