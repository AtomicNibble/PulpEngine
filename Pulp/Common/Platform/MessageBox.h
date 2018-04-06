#pragma once

#ifndef X_PLATFORM_MSGBOX_H_
#define X_PLATFORM_MSGBOX_H_

X_NAMESPACE_BEGIN(core)

namespace msgbox
{
    // not using engine enums as i don't think i'll ever want ToString for these.

    X_DECLARE_FLAGS(Style)
    (
        Info,
        Warning,
        Error,
        Question,

        Topmost,
        DefaultDesktop);

    typedef Flags<Style> StyleFlags;

    enum class Buttons
    {
        OK,
        OKCancel,
        YesNo,
        Quit
    };

    enum class Selection
    {
        OK,
        Cancel,
        Yes,
        No,
        None,
        Error
    };

    X_DECLARE_FLAG_OPERATORS(StyleFlags);

    const StyleFlags DEFAULT_STYLE = Style::Info;
    const Buttons DEFAULT_BUTTONS = Buttons::OK;

    Selection show(const char* pMessage, const char* pTitle, StyleFlags style, Buttons buttons);
    Selection show(const wchar_t* pMessage, const wchar_t* pTitle, StyleFlags style, Buttons buttons);

    inline Selection show(const char* pMessage, const char* pTitle, StyleFlags style)
    {
        return show(pMessage, pTitle, style, DEFAULT_BUTTONS);
    }

    inline Selection show(const char* pMessage, const char* pTitle, Buttons buttons)
    {
        return show(pMessage, pTitle, DEFAULT_STYLE, buttons);
    }

    inline Selection show(const char* pMessage, const char* pTitle)
    {
        return show(pMessage, pTitle, DEFAULT_STYLE, DEFAULT_BUTTONS);
    }

    inline Selection show(const wchar_t* pMessage, const wchar_t* pTitle, StyleFlags style)
    {
        return show(pMessage, pTitle, style, DEFAULT_BUTTONS);
    }

    inline Selection show(const wchar_t* pMessage, const wchar_t* pTitle, Buttons buttons)
    {
        return show(pMessage, pTitle, DEFAULT_STYLE, buttons);
    }

    inline Selection show(const wchar_t* pMessage, const wchar_t* pTitle)
    {
        return show(pMessage, pTitle, DEFAULT_STYLE, DEFAULT_BUTTONS);
    }

} // namespace msgbox

X_NAMESPACE_END

#endif // !X_PLATFORM_MSGBOX_H_