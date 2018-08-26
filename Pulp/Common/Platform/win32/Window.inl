
X_INLINE uint32_t xWindow::GetClientWidth(void) const
{
    return safe_static_cast<uint32_t>(GetClientRect().getWidth());
}

X_INLINE uint32_t xWindow::GetClientHeight(void) const
{
    return safe_static_cast<uint32_t>(GetClientRect().getHeight());
}

X_INLINE PLATFORM_HWND xWindow::GetNativeWindow(void)
{
    return window_;
}

X_INLINE const PLATFORM_HWND xWindow::GetNativeWindow(void) const
{
    return window_;
}

X_INLINE bool xWindow::Hasfocus(void) const
{
    return ::GetForegroundWindow() == window_;
}

X_INLINE void xWindow::Show(void)
{
    ShowWindow(window_, SW_SHOW);
}

X_INLINE void xWindow::Hide(void)
{
    ShowWindow(window_, SW_HIDE);
}

X_INLINE void xWindow::Minamise(void)
{
    ShowWindow(window_, SW_MINIMIZE);
}

X_INLINE void xWindow::Maximise(void)
{
    ShowWindow(window_, SW_MAXIMIZE);
}

X_INLINE void xWindow::Restore(void)
{
    ShowWindow(window_, SW_RESTORE);
}

X_INLINE void xWindow::Close(void)
{
    ::PostMessage(window_, WM_CLOSE, 0, 0);
}

X_INLINE void xWindow::SetTitle(const char* str)
{
    ::SetWindowTextA(window_, str);
}

X_INLINE void xWindow::HideClientCursor(bool hide)
{
    hideClientCursor_ = hide;
}

X_INLINE void xWindow::FixedAspectRatioSizing(bool enable)
{
    sizingFixedAspectRatio_ = enable;
}

X_INLINE bool xWindow::HasFocus(void) const
{
    return hasFocus_;
}

X_INLINE const uint32_t xWindow::GetNumMsgs(void) const
{
    return numMsgs_;
}

X_INLINE const uint32_t xWindow::GetNumMsgsClear(void)
{
    uint32_t ret = numMsgs_;
    numMsgs_ = 0;
    return ret;
}

X_INLINE bool xWindow::isDebugEnable(void)
{
    return s_var_windowDebug != 0;
}
