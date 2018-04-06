#pragma once

#include <Shellapi.h>

X_NAMESPACE_BEGIN(core)

#ifdef RegisterClass
#undef RegisterClass
#endif // !RegisterClass

class TrayIcon
{
    static uint32_t CREATED_MSG;

public:
    TrayIcon();
    ~TrayIcon();

    bool CreateIcon(HWND hParent, LPCTSTR toolTip, uint32_t iconId, uint32_t menuID, bool bHidden = false);
    void DestoryIcon(void);
    void RemoveIcon(void);

    virtual LRESULT OnTrayNotification(UINT msg, WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnTrayCmd(WPARAM wParam, LPARAM lParam);

private:
    void RegisterClass(void);
    void UnRegisterClass(void);

    uint32_t GetCallbackMessage(void) const;

public:
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT OnTaskbarCreated(WPARAM wParam, LPARAM lParam);

private:
    X_NO_COPY(TrayIcon);
    X_NO_ASSIGN(TrayIcon);

private:
    NOTIFYICONDATA tnd_;

    bool showIconPending_; // Show the icon once tha taskbar has been created
    bool hidden_;          // Has the icon been hidden?
    bool created_;
    uint32_t defaultMenuItemByPos_;
    uint32_t defaultMenuItemID_;

    HWND hWnd_;
    HWND targetWnd_;
    HINSTANCE hInstance_;

private:
    static core::AtomicInt regCount_;
    static core::AtomicInt classRegisterd_;
};

X_NAMESPACE_END