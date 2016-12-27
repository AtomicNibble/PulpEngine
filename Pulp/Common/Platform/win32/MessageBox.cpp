#include "EngineCommon.h"
#include "../MessageBox.h"


X_NAMESPACE_BEGIN(core)


namespace msgbox
{

	namespace 
	{

		UINT getIcon(Style style)
		{
			switch (style) {
				case Style::Info:
					return MB_ICONINFORMATION;
				case Style::Warning:
					return MB_ICONWARNING;
				case Style::Error:
					return MB_ICONERROR;
				case Style::Question:
					return MB_ICONQUESTION;
				default:
					return MB_ICONINFORMATION;
			}
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



	Selection show(const char* pMessage, const char* pTitle, Style style, Buttons buttons)
	{
		UINT flags = MB_TASKMODAL;

		flags |= getIcon(style);
		flags |= getButtons(buttons);

		return getSelection(MessageBoxA(nullptr, pMessage, pTitle, flags));
	}

	Selection show(const wchar_t* pMessage, const wchar_t* pTitle, Style style, Buttons buttons)
	{
		UINT flags = MB_TASKMODAL;

		flags |= getIcon(style);
		flags |= getButtons(buttons);

		return getSelection(MessageBoxW(nullptr, pMessage, pTitle, flags));
	}


} // namespace msgbox


X_NAMESPACE_END