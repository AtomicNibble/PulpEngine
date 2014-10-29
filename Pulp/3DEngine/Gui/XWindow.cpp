#include "stdafx.h"
#include "XWindow.h"

X_NAMESPACE_BEGIN(gui)


const char* XWindow::RegisterVars[] =
{
	{ "forecolor" },
	{ "hovercolor" },
	{ "backcolor" },
	{ "bordercolor" },
	{ "rect" },
	{ "textscale" },
	{ "visible" },
	{ "text" },
};


const char* XWindow::ScriptNames[XWindow::Event::ENUM_COUNT] = {
	"onMouseEnter",
	"onMouseExit",
	"onESC",
	"onEnter",
};


const int XWindow::NumRegisterVars = sizeof(RegisterVars) / sizeof(const char*);



XWindow::XWindow() :
	children_(g_3dEngineArena)
{

}

XWindow::~XWindow()
{

}




// ----------------------------------------

// Drawing
void XWindow::reDraw(void)
{

}

void XWindow::drawDebug(void)
{

}

void XWindow::drawCaption(void)
{

}


// -------------- Overrides ---------------

bool XWindow::Parse(core::XLexer& lex)
{
	// we have a { }
	core::XLexToken token;

	if (!lex.ExpectTokenString("{"))
		return false;

	if (!lex.ReadToken(token))
		return false;

	// read what we got in the brace baby.
	while (!token.isEqual("}"))
	{



		if (!lex.ReadToken(token))
		{
			X_ERROR("Gui", "error while parsing windowDef");
			return false;
		}
	}

	return true;
}

void XWindow::draw(int time, float x, float y)
{

}

void XWindow::activate(bool activate)
{

}

void XWindow::gainFocus(void)
{

}

void XWindow::loseFocus(void)
{

}

void XWindow::gainCapture(void)
{

}

void XWindow::loseCapture(void)
{

}

void XWindow::sized(void)
{

}

void XWindow::moved(void)
{

}

void XWindow::mouseExit(void)
{

}

void XWindow::mouseEnter(void)
{

}



X_NAMESPACE_END