#include "stdafx.h"
#include "Gui.h"

#include <String\Lexer.h>

#include <IFileSys.h>

#include "XWindow.h"


X_NAMESPACE_BEGIN(gui)


XGui::XGui() : 
	pDesktop_(nullptr)
{

}

XGui::~XGui()
{

}

void XGui::Redraw(int time, bool hud)
{

}

void XGui::DrawCursor(void)
{

}

const char* XGui::Activate(bool activate, int time)
{


	return nullptr;
}


// -------------------------------------------

bool XGui::InitFromFile(const char* name)
{
	core::Path path;
	core::XFileMemScoped file;
	core::fileModeFlags mode;

	// already init?
	if (isDeskTopValid())
	{
		X_WARNING("Gui", "gui item is already init from: \"%s\"",
			name_.c_str());
		return true;
	}

	pDesktop_ = X_NEW(XWindow, g_3dEngineArena, "MenuWindow");

	path = "gui/compiled/";
	path.setFileName(name);
	path.setExtension(GUI_BINARY_FILE_EXTENSION);

	mode.Set(core::fileMode::READ);

	if (file.openFile(path.c_str(), mode))
	{
		// load the compiled version.
		X_LOG0("Gui", "loading: \"%s\"", path.c_str());
		return false;
	}

	// try none binary
	path = "gui/";
	path.setFileName(name);
	path.setExtension(GUI_FILE_EXTENSION);
	if (file.openFile(path.c_str(), mode))
	{
		X_LOG0("Gui", "parsing: \"%s\"", path.c_str());
		return ParseTextFile(file->getBufferStart(), file->getBufferEnd());
	}

	X_DELETE_AND_NULL(pDesktop_, g_3dEngineArena);
	X_ERROR("Gui", "failed to open gui file: \"%s\"", path.c_str());
	return false;
}


bool XGui::ParseTextFile(const char* begin, const char* end)
{
	core::XLexer lex(begin,end);

	// we have a window def first.
	if (lex.ExpectTokenString("windowDef"))
	{

		return pDesktop_->Parse(lex);

	}

	return false;
}

X_NAMESPACE_END