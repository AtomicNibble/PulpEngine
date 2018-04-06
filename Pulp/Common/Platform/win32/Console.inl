

inline HANDLE Console::GetNativeConsole(void) const
{
    return console_;
}

inline HWND Console::GetNativeConsoleWindow(void) const
{
    return window_;
}
