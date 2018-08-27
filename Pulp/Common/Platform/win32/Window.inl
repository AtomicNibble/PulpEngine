
X_INLINE uint32_t Window::GetClientWidth(void) const
{
    return safe_static_cast<uint32_t>(GetClientRect().getWidth());
}

X_INLINE uint32_t Window::GetClientHeight(void) const
{
    return safe_static_cast<uint32_t>(GetClientRect().getHeight());
}

X_INLINE PLATFORM_HWND Window::GetNativeWindow(void)
{
    return window_;
}

X_INLINE const PLATFORM_HWND Window::GetNativeWindow(void) const
{
    return window_;
}

X_INLINE bool Window::Hasfocus(void) const
{
    return ::GetForegroundWindow() == window_;
}

X_INLINE void Window::Show(void)
{
    ::ShowWindow(window_, SW_SHOW);
}

X_INLINE void Window::Hide(void)
{
    ::ShowWindow(window_, SW_HIDE);
}

X_INLINE void Window::Minamise(void)
{
    ::ShowWindow(window_, SW_MINIMIZE);
}

X_INLINE void Window::Maximise(void)
{
    ::ShowWindow(window_, SW_MAXIMIZE);
}

X_INLINE void Window::Restore(void)
{
    ::ShowWindow(window_, SW_RESTORE);
}

X_INLINE void Window::Close(void)
{
    ::PostMessage(window_, WM_CLOSE, 0, 0);
}

X_INLINE void Window::SetTitle(const char* str)
{
    ::SetWindowTextA(window_, str);
}

X_INLINE void Window::HideClientCursor(bool hide)
{
    hideClientCursor_ = hide;
}

X_INLINE void Window::FixedAspectRatioSizing(bool enable)
{
    sizingFixedAspectRatio_ = enable;
}

X_INLINE bool Window::HasFocus(void) const
{
    return hasFocus_;
}

X_INLINE bool Window::isMaximized(void) const
{
    return maximized_;
}

X_INLINE Window::Mode::Enum Window::getMode(void) const
{
    return mode_;
}

X_INLINE const uint32_t Window::GetNumMsgs(void) const
{
    return numMsgs_;
}

X_INLINE const uint32_t Window::GetNumMsgsClear(void)
{
    uint32_t ret = numMsgs_;
    numMsgs_ = 0;
    return ret;
}

X_INLINE bool Window::isDebugEnable(void)
{
    return s_var_windowDebug != 0;
}
