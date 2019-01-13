#include "EngineCommon.h"
#include "TrayIcon.h"

X_NAMESPACE_BEGIN(core)

namespace
{
    static const wchar_t* g_TrayClassName = L"TrayClass";

    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        LONG_PTR data = GetWindowLongPtr(hWnd, GWLP_USERDATA); // GetWindowLongPtr required for 64

        return reinterpret_cast<TrayIcon*>(data)->WndProc(hWnd, msg, wParam, lParam);
    }

    LRESULT CALLBACK PreWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE) {
            LPCREATESTRUCT pInfo = reinterpret_cast<LPCREATESTRUCT>(lParam);
            TrayIcon* pClass = reinterpret_cast<TrayIcon*>(pInfo->lpCreateParams);

            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pClass));
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));

            return pClass->WndProc(hWnd, msg, wParam, lParam);
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

} // namespace

uint32_t TrayIcon::CREATED_MSG = 0;
core::AtomicInt TrayIcon::regCount_;
core::AtomicInt TrayIcon::classRegisterd_;

TrayIcon::TrayIcon() :
    showIconPending_(false),
    hidden_(false),
    created_(false),

    defaultMenuItemByPos_(1),
    defaultMenuItemID_(0),

    hWnd_(nullptr),
    targetWnd_(nullptr),
    hInstance_(nullptr)
{
    core::zero_object(tnd_);

    hInstance_ = ::GetModuleHandleW(nullptr);

    RegisterClass();
}

TrayIcon::~TrayIcon()
{
    DestoryIcon();

    UnRegisterClass();
}

bool TrayIcon::CreateIcon(HWND hParent, core::string_view toolTip, uint32_t iconId, uint32_t menuID, bool bHidden)
{
    HICON icon = ::LoadIcon(hInstance_, MAKEINTRESOURCE(iconId));
    if (!icon) {
        core::lastError::Description Dsc;
        X_ERROR("TrayIcon", "Failed to load icon %i for tray icon. Err: %s", iconId, core::lastError::ToString(Dsc));
        return false;
    }

    hWnd_ = ::CreateWindowW(g_TrayClassName, L"", WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, 0,
        hInstance_,
        this);

    if (!hWnd_) {
        core::lastError::Description Dsc;
        X_ERROR("TrayIcon", "Failed to create window for tray icon. Err: %s", core::lastError::ToString(Dsc));
        return false;
    }

    // load up the NOTIFYICONDATA structure
    tnd_.cbSize = sizeof(NOTIFYICONDATA);
    tnd_.hWnd = (hParent) ? hParent : hWnd_;
    tnd_.uID = menuID;
    tnd_.hIcon = icon;
    tnd_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnd_.uCallbackMessage = ::RegisterWindowMessage(L"WM_TRAYICON");

    core::strUtil::Convert(toolTip, tnd_.szTip);

    hidden_ = bHidden;      // Starts hidden ?
    targetWnd_ = tnd_.hWnd; // Save target hWnd.

    if (hidden_) {
        tnd_.uFlags = NIF_STATE;
        tnd_.dwState = NIS_HIDDEN;
        tnd_.dwStateMask = NIS_HIDDEN;
    }

    if (!Shell_NotifyIcon(NIM_ADD, &tnd_)) {
        core::lastError::Description Dsc;
        X_ERROR("TrayIcon", "Failed to create tray icon. Err: %s", core::lastError::ToString(Dsc));
        return false;
    }

    created_ = true;
    showIconPending_ = hidden_;
    return true;
}

void TrayIcon::DestoryIcon(void)
{
    RemoveIcon();

    if (hWnd_) {
        ::DestroyWindow(hWnd_);
        hWnd_ = nullptr;
    }

    UnRegisterClass();
}

void TrayIcon::RemoveIcon(void)
{
    showIconPending_ = false;

    if (created_) {
        created_ = false;
        tnd_.uFlags = 0;
        if (!Shell_NotifyIcon(NIM_DELETE, &tnd_)) {
            core::lastError::Description Dsc;
            X_ERROR("TrayIcon", "Failed to remove icon. Err: %s", core::lastError::ToString(Dsc));
        }
    }
    core::zero_object(tnd_);
}

void TrayIcon::RegisterClass(void)
{
    if (regCount_ > 0 || classRegisterd_ > 0) {
        return;
    }

    // Register the class
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.lpfnWndProc = PreWndProc; // PreWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance_;
    wcex.hIcon = 0;
    wcex.hCursor = 0;
    wcex.hbrBackground = 0;
    wcex.lpszMenuName = 0;
    wcex.lpszClassName = g_TrayClassName;
    wcex.hIconSm = 0;

    if (RegisterClassExW(&wcex) == 0) {
        core::lastError::Description Dsc;
        X_ERROR("TrayIcon", "Failed to remove icon. Err: %s", core::lastError::ToString(Dsc));
    }
    else {
        ++regCount_;
        classRegisterd_ = 1;
    }
}

void TrayIcon::UnRegisterClass(void)
{
    --regCount_;

    if (regCount_ > 0 || classRegisterd_ == 0) {
        return;
    }

    if (!UnregisterClassW(g_TrayClassName, hInstance_)) {
        core::lastError::Description Dsc;
        X_ERROR("TrayIcon", "Failed to unregister class. Err: %s", core::lastError::ToString(Dsc));
    }

    classRegisterd_ = 0;
}

uint32_t TrayIcon::GetCallbackMessage(void) const
{
    return tnd_.uCallbackMessage;
}

LRESULT TrayIcon::OnTaskbarCreated(WPARAM wParam, LPARAM lParam)
{
    X_UNUSED(wParam);
    X_UNUSED(lParam);
    return 0L;
}

LRESULT TrayIcon::OnTrayNotification(UINT msg, WPARAM uID, LPARAM lEvent)
{
    X_UNUSED(msg);

    if (tnd_.uID == 0 || uID != tnd_.uID) {
        return 0L;
    }

    if (!targetWnd_) {
        return 0L;
    }

    if (LOWORD(lEvent) == WM_RBUTTONUP) {
        core::lastError::Description Dsc;

        HMENU hMenu = ::LoadMenu(hInstance_, MAKEINTRESOURCE(tnd_.uID));
        if (!hMenu) {
            X_ERROR("TrayIcon", "Failed to load menu %i Err: %s", tnd_.uID, core::lastError::ToString(Dsc));
            return 0;
        }

        HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
        if (!hSubMenu) {
            X_ERROR("TrayIcon", "Failed to load sub-menu %i:0 Err: %s", tnd_.uID, core::lastError::ToString(Dsc));
            ::DestroyMenu(hMenu);
            return 0;
        }

        // Make chosen menu item the default (bold font)
        if (!::SetMenuDefaultItem(hSubMenu, defaultMenuItemID_, defaultMenuItemByPos_)) {
            X_ERROR("TrayIcon", "Failed to set default menu item %i Err: %s",
                defaultMenuItemID_, core::lastError::ToString(Dsc));
            ::DestroyMenu(hMenu);
            return 0;
        }

        // Display and track the popup menu
        POINT pos;
        GetCursorPos(&pos);

        if (!::SetForegroundWindow(tnd_.hWnd)) {
            X_ERROR("TrayIcon", "Failed to set forground menu hwnd: 0x%x Err: %s",
                tnd_.hWnd, core::lastError::ToString(Dsc));
        }
        if (!::TrackPopupMenu(hSubMenu, 0, pos.x, pos.y, 0, targetWnd_, NULL)) {
            X_ERROR("TrayIcon", "Failed to trak menu Err: %s", core::lastError::ToString(Dsc));
        }

        // BUGFIX: See "PRB: Menus for Notification Icons Don't Work Correctly"
        ::PostMessage(tnd_.hWnd, WM_NULL, 0, 0);

        DestroyMenu(hMenu);
    }
    else if (LOWORD(lEvent) == WM_LBUTTONDBLCLK) {
        // double click received, the default action is to execute default menu item
        ::SetForegroundWindow(tnd_.hWnd);

        UINT uItem;
        if (defaultMenuItemByPos_) {
            core::lastError::Description Dsc;

            HMENU hMenu = ::LoadMenu(hInstance_, MAKEINTRESOURCE(tnd_.uID));
            if (!hMenu) {
                X_ERROR("TrayIcon", "Failed to load menu %i Err: %s", tnd_.uID, core::lastError::ToString(Dsc));
                return 0;
            }

            HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
            if (!hSubMenu) {
                X_ERROR("TrayIcon", "Failed to load sub-menu %i:0 Err: %s", tnd_.uID, core::lastError::ToString(Dsc));
                return 0;
            }

            uItem = ::GetMenuItemID(hSubMenu, defaultMenuItemID_);

            DestroyMenu(hMenu);
        }
        else {
            uItem = defaultMenuItemID_;
        }

        ::PostMessage(targetWnd_, WM_COMMAND, uItem, 0);
    }

    return 1;
}

LRESULT TrayIcon::OnTrayCmd(WPARAM wParam, LPARAM lParam)
{
    X_UNUSED(wParam);
    X_UNUSED(lParam);
    return 0;
}

LRESULT TrayIcon::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == GetCallbackMessage()) {
        return OnTrayNotification(msg, wp, lp);
    }

    if (msg == TrayIcon::CREATED_MSG) {
        return OnTaskbarCreated(wp, lp);
    }

    if (msg == WM_COMMAND && hWnd == targetWnd_) {
        OnTrayCmd(wp, lp);
    }

    return ::DefWindowProc(hWnd, msg, wp, lp);
}

X_NAMESPACE_END