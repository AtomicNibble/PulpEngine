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

void XGui::Redraw()
{
	if (pDesktop_)
		pDesktop_->reDraw();
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

	// TODO remove extension.?
	this->name_ = name;

	mode.Set(core::fileMode::READ);


	// already init?
	if (isDeskTopValid())
	{
		// we are reloading most likley, warn if diffrent.
		if (name_ != name)
		{
			X_WARNING("Gui", "gui item is already init for menu: \"%s\"",
				name_.c_str());
		}

		// just delete the last one?
		// would be cool if like state was preserved across a reload.
		// so colors and sizes could be changed, and moving items still
		// be on same path.
		X_DELETE_AND_NULL(pDesktop_, g_3dEngineArena);

		pDesktop_ = X_NEW(XWindow, g_3dEngineArena, "MenuWindow");
		pDesktop_->setFlag(WindowFlag::DESKTOP);
		// for reloading do we want to ignore compiled ones?
		// i think so.
		goto SourceLoad;
	}

	pDesktop_ = X_NEW(XWindow, g_3dEngineArena, "MenuWindow");
	pDesktop_->setFlag(WindowFlag::DESKTOP);

	path = "gui/compiled/";
	path.setFileName(name);
	path.setExtension(GUI_BINARY_FILE_EXTENSION);

	if (file.openFile(path.c_str(), mode))
	{
		// load the compiled version.
		X_LOG0("Gui", "loading: \"%s\"", path.c_str());
		return false;
	}

SourceLoad:

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
	core::XLexer::LexFlags flags;

	flags.Set(core::LexFlag::NOFATALERRORS);
	flags.Set(core::LexFlag::NOSTRINGCONCAT);
	flags.Set(core::LexFlag::ALLOWMULTICHARLITERALS);
	flags.Set(core::LexFlag::ALLOWBACKSLASHSTRINGCONCAT);

	lex.setFlags(flags);

	// we have a window def first.
	if (lex.ExpectTokenString("windowDef"))
	{
		if (pDesktop_->Parse(lex))
		{
			pDesktop_->FixUpParms();
			return true;
		}
	}

	return false;
}

X_NAMESPACE_END