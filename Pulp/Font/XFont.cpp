#include "stdafx.h"

#include "XFont.h"
#include "Font.h"
#include "XFontTexture.h"

#include <String\StackString.h>

#include <IConsole.h>

X_NAMESPACE_BEGIN(font)

namespace
{

	void Command_ListFonts(core::IConsoleCmdArgs* pCmd)
	{
		X_UNUSED(pCmd);
		XFont* pFont = (XFont*)gEnv->pFont;

		pFont->ListFontNames();
	}

	void Command_DumpForName(core::IConsoleCmdArgs* pCmd)
	{
		XFont* pFont = (XFont*)gEnv->pFont;

		int Num = pCmd->GetArgCount();

		if (Num < 2)
		{
			X_WARNING("Console", "fonts_dump_for_name <name>");
			return;
		}

		const char* name = pCmd->GetArg(1);
		XFFont* font = (XFFont*)pFont->GetFont(name);
		if (font)
		{
			if (font->getFontTexture()->WriteToFile(name))
			{
				X_LOG0("Font", "^8font texture successfully dumped!");
			}
		}
		else
		{
			X_ERROR("Font", "failed to dump font, no font exsists for name: %s",
				name);
		}
	}

}

XFont::XFont(ICore* pCore) :
	pCore_(pCore),
	fonts_(g_fontArena, 6)
{
	X_ASSERT_NOT_NULL(pCore);
	X_ASSERT_NOT_NULL(g_fontArena);

	Init();
}

XFont::~XFont()
{

}

void XFont::release()
{
	X_DELETE(this,g_fontArena);
}


void XFont::Init()
{
	// add font commands
	ADD_COMMAND("font_list_loaded", Command_ListFonts, core::VarFlag::SYSTEM, 
		"Lists all the loaded fonts");

	ADD_COMMAND("font_dump_for_name", Command_DumpForName, core::VarFlag::SYSTEM,
		"Dumps the font texture for a given name");

	gEnv->pHotReload->addfileType(this, "font");
}

void XFont::ShutDown()
{
	X_LOG0("FontSys", "Shutting Down");

	FontMapItor it = fonts_.begin();
	for (; it != fonts_.end(); ++it) {
		X_DELETE(it->second, g_fontArena);
	}

	gEnv->pHotReload->addfileType(nullptr, "font");


	fonts_.clear();
}


IFFont* XFont::NewFont(const char* pFontName)
{
	FontMapItor it = fonts_.find(X_CONST_STRING(pFontName));
	if (it != fonts_.end())
		return it->second;

	XFFont* pFont = X_NEW(XFFont, g_fontArena, "FontObject")(pCore_, this, pFontName);
	fonts_.insert(FontMap::value_type(core::string(pFontName), pFont));
	return pFont;
}

IFFont* XFont::GetFont(const char* pFontName) const
{
	FontMapConstItor it = fonts_.find(X_CONST_STRING(pFontName));
	return it != fonts_.end() ? it->second : 0;
}

void XFont::ListFontNames(void) const
{
	FontMapConstItor it = fonts_.begin();

	X_LOG0("Fonts", "---------------- ^8Fonts^7 ----------------");
	for (; it != fonts_.end(); ++it)
	{
		XFontTexture* pTex = it->second->getFontTexture();
		if (pTex) {
			X_LOG0("Fonts", "Name: %s, Size: (%i,%i), Usage: %i", it->second->getName(),
				pTex->GetWidth(), pTex->GetHeight(), pTex->GetSlotUsage());
		}
		else {
			X_LOG0("Fonts", "Name: %s", it->second->getName());
		}
	}
	
	X_LOG0("Fonts", "-------------- ^8Fonts End^7 --------------");
}

bool XFont::OnFileChange(const char* name)
{
	using namespace core;

	Path<char> path(name);
	if (strUtil::IsEqual(".font", path.extension()))
	{
		path.removeExtension();

		XFFont* pFont = (XFFont*)GetFont(path.fileName());
		if (pFont)
		{
			pFont->Reload();
		}
		return true;
	}

	return false;
}

X_NAMESPACE_END
