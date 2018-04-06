
X_NAMESPACE_BEGIN(core)

X_INLINE void XConsole::ShowConsole(consoleState::Enum state)
{
    if (state == consoleState::CLOSED) {
        ResetHistoryPos();
    }

    consoleState_ = state;
}

X_INLINE bool XConsole::isVisable(void) const
{
    return consoleState_ != consoleState::CLOSED;
}

X_INLINE bool XConsole::isExpanded(void) const
{
    return consoleState_ == consoleState::EXPANDED;
}

X_INLINE void XConsole::ToggleConsole(bool expand)
{
    if (isVisable()) {
        consoleState_ = consoleState::CLOSED;
        ResetHistoryPos();
    }
    else {
        if (expand)
            consoleState_ = consoleState::EXPANDED;
        else
            consoleState_ = consoleState::OPEN;
    }
}

X_INLINE bool XConsole::isAutocompleteVis(void)
{
    return autoCompleteNum_ > 0;
}

X_NAMESPACE_END
