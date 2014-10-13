
X_INLINE unsigned int xWindow::GetClientWidth(void) const
{
	return safe_static_cast<unsigned int>(GetClientRect().getWidth());
}


X_INLINE unsigned int xWindow::GetClientHeight(void) const
{
	return safe_static_cast<unsigned int>(GetClientRect().getHeight());
}

/*
X_INLINE void xWindow::Bind(InputEvent::Sink* sink)
{
	m_inputEvent.Bind(sink);
}


X_INLINE void xWindow::Bind(RawInputEvent::Sink* sink)
{
	m_rawInputEvent.Bind(sink);
}*/


X_INLINE HWND xWindow::GetNativeWindow(void)
{
	return m_window;
}


X_INLINE const HWND xWindow::GetNativeWindow(void) const
{
	return m_window;
}

X_INLINE bool xWindow::Hasfocus(void) const
{
	return ::GetForegroundWindow() == this->m_window;
}

X_INLINE void xWindow::Show(void)
{
	ShowWindow(m_window, SW_SHOW);
}

X_INLINE void xWindow::Hide(void)
{
	ShowWindow(m_window, SW_HIDE);
}

X_INLINE void xWindow::Minamise(void)
{
	ShowWindow(m_window, SW_MINIMIZE);
}

X_INLINE void xWindow::MaxiMise(void)
{
	ShowWindow(m_window, SW_MAXIMIZE);
}

X_INLINE void xWindow::Restore(void)
{
	ShowWindow(m_window, SW_RESTORE);
}

X_INLINE void xWindow::Close(void)
{
	::PostMessage(m_window, WM_CLOSE, 0, 0);
}

X_INLINE void xWindow::SetTitle(const char* str)
{
	::SetWindowTextA(this->m_window, str);
}

X_INLINE void xWindow::Destroy(void)
{
	DestroyWindow(m_window);
}
