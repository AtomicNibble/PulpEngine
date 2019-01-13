#include "EngineCommon.h"
#include "../MessageBox.h"

X_NAMESPACE_BEGIN(core)

namespace msgbox
{
    namespace
    {
        UINT getFlags(StyleFlags style)
        {
            UINT flags = 0;

            if (style.IsSet(Style::Topmost)) {
                flags |= MB_TOPMOST;
            }
            if (style.IsSet(Style::DefaultDesktop)) {
                flags |= MB_DEFAULT_DESKTOP_ONLY;
            }

            return flags;
        }

        UINT getIcon(StyleFlags style)
        {
            if (style.IsSet(Style::Info)) {
                return MB_ICONINFORMATION;
            }
            if (style.IsSet(Style::Warning)) {
                return MB_ICONWARNING;
            }
            if (style.IsSet(Style::Error)) {
                return MB_ICONERROR;
            }
            if (style.IsSet(Style::Question)) {
                return MB_ICONQUESTION;
            }

            return MB_ICONINFORMATION;
        }

        UINT getButtons(Buttons buttons)
        {
            switch (buttons) {
                case Buttons::OK:
                case Buttons::Quit: // There is no 'Quit' button on windows :(
                    return MB_OK;
                case Buttons::OKCancel:
                    return MB_OKCANCEL;
                case Buttons::YesNo:
                    return MB_YESNO;
                default:
                    return MB_OK;
            }
        }

        Selection getSelection(int32_t response)
        {
            switch (response) {
                case IDOK:
                    return Selection::OK;
                case IDCANCEL:
                    return Selection::Cancel;
                case IDYES:
                    return Selection::Yes;
                case IDNO:
                    return Selection::No;
                default:
                    return Selection::None;
            }
        }

    } // namespace

    Selection show(core::string_view msg, core::string_view title, StyleFlags style, Buttons buttons)
    {
        UINT flags = MB_TASKMODAL;

        flags |= getIcon(style);
        flags |= getFlags(style);
        flags |= getButtons(buttons);

        core::StackStringW256 msgW(msg.begin(), msg.end());
        core::StackStringW256 titleW(title.begin(), title.end());

        return getSelection(MessageBoxW(nullptr, msgW.c_str(), titleW.c_str(), flags));
    }

} // namespace msgbox

X_NAMESPACE_END