#include "stdafx.h"

#include "XFontSystem.h"
#include "Font.h"
#include "FontRender\XFontTexture.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(font)

namespace
{

	void Command_ListFonts(core::IConsoleCmdArgs* pCmd)
	{
		X_UNUSED(pCmd);
		XFontSystem* pFont = static_cast<XFontSystem*>(gEnv->pFontSys);

		pFont->ListFonts();
	}

	void Command_DumpForName(core::IConsoleCmdArgs* pCmd)
	{
		XFontSystem* pFont = static_cast<XFontSystem*>(gEnv->pFontSys);

		size_t Num = pCmd->GetArgCount();

		if (Num < 2)
		{
			X_WARNING("Console", "fonts_dump_for_name <name>");
			return;
		}

		const char* name = pCmd->GetArg(1);
		XFont* font = static_cast<XFont*>(pFont->GetFont(name));
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

XFontSystem::XFontSystem(ICore* pCore) :
	pCore_(pCore),
	fonts_(g_fontArena, 6),
	lock_(10),
	fontTextures_(g_fontArena, 4)
{
	X_ASSERT_NOT_NULL(pCore);
	X_ASSERT_NOT_NULL(g_fontArena);

}

XFontSystem::~XFontSystem()
{

}

void XFontSystem::release(void)
{
	X_DELETE(this,g_fontArena);
}

void XFontSystem::registerVars(void)
{
	vars_.registerVars();
}

void XFontSystem::registerCmds(void)
{
	// add font commands
	ADD_COMMAND("listFonts", Command_ListFonts, core::VarFlag::SYSTEM,
		"Lists all the loaded fonts");

	ADD_COMMAND("fontDumpForMame", Command_DumpForName, core::VarFlag::SYSTEM,
		"Dumps the font texture for a given font name");

}


bool XFontSystem::init(void)
{
	X_LOG0("FontSys", "Starting");


	gEnv->pHotReload->addfileType(this, "font");
	gEnv->pHotReload->addfileType(this, "ttf");

	return true;
}

void XFontSystem::shutDown(void)
{
	X_LOG0("FontSys", "Shutting Down");

	FontMapItor it = fonts_.begin();
	for (; it != fonts_.end(); ++it) {
		X_DELETE(it->second, g_fontArena);
	}
	fonts_.clear();


	gEnv->pHotReload->addfileType(nullptr, "font");
	gEnv->pHotReload->addfileType(nullptr, "ttf");


}


bool XFontSystem::asyncInitFinalize(void)
{
	// potentiall we will hold this lock for a while.
	// but we should not really see any contention.
	core::CriticalSection::ScopedLock lock(lock_); 

	bool result = true;

	for (const auto& fontIt : fonts_)
	{
		auto* pFont = fontIt.second;
		
		// we call all even if one fails.
		result &= pFont->WaitTillReady();
	}

	return result;
}

void XFontSystem::appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const
{
	core::CriticalSection::ScopedLock lock(lock_);

	for (const auto& fontIt : fonts_)
	{
		auto* pFont = fontIt.second;

		if (pFont->isDirty())
		{
			pFont->appendDirtyBuffers(bucket);
		}
	}
}


IFont* XFontSystem::NewFont(const char* pFontName)
{	
	{
		IFont* pFont;
		if ((pFont = GetFont(pFontName)) != nullptr) {
			return pFont;
		}
	}

	XFont* pFont = X_NEW(XFont, g_fontArena, "FontObject")(*this, pFontName);
	fonts_.insert(FontMap::value_type(core::string(pFontName), pFont));
	return pFont;
}

IFont* XFontSystem::GetFont(const char* pFontName) const
{
	auto it = fonts_.find(SourceNameStr(pFontName));
	if (it != fonts_.end()) {
		return it->second;
	}

	return nullptr;
}

XFontTexture* XFontSystem::getFontTexture(const SourceNameStr& name, bool async)
{
	core::CriticalSection::ScopedLock lock(lock_);

	auto it = fontTextures_.find(name);
	if (it != fontTextures_.end())
	{
		it->second->addReference();
		return it->second;
	}

	XFontTexture* pFontTexture = X_NEW(XFontTexture, g_fontArena, "FontTexture")(name, vars_, g_fontArena);

	// setup the font texture cpu buffers.
	if (!pFontTexture->Create(512, 512, 16, 16)) 
	{
		X_DELETE(pFontTexture, g_fontArena);
		return nullptr;
	}

	// the font cache's are now valid but we have not yet loaded the font glyph file into the font renderer.
	// so we can't actually create any glyphs yet.
	// the font file is optionally loaded in the background so it may be a few frames before this fontTexture is usable.
	// you must check with 'IsReady'
	if (!pFontTexture->LoadGlyphSource(async))
	{
		X_DELETE(pFontTexture, g_fontArena);
		return nullptr;
	}

	if (!async)
	{
		X_ASSERT(pFontTexture->IsReady(), "Should be ready if loaded none async")(async, pFontTexture->IsReady());
	}

	fontTextures_.insert(std::make_pair(name, pFontTexture));
	return pFontTexture;
}

void XFontSystem::releaseFontTexture(XFontTexture* pFontTex)
{
	if (!pFontTex) {
		return;
	}

	core::CriticalSection::ScopedLock lock(lock_);

	auto it = fontTextures_.find(pFontTex->GetName());
	if (it == fontTextures_.end())
	{
		X_ERROR("Font", "Failed to find FontTexture for removal. name: \"%s\"", pFontTex->GetName().c_str());
		return;
	}

	if (pFontTex->removeReference() == 0)
	{
//		fontTextures_.erase(it);
		X_DELETE(pFontTex, g_fontArena);
	}
}




void XFontSystem::ListFonts(void) const
{
	FontMapConstItor it = fonts_.begin();
	FontFlags::Description FlagDesc;

	X_LOG0("Fonts", "---------------- ^8Fonts^7 ---------------");
	for (; it != fonts_.end(); ++it)
	{
		XFont* pFont = it->second;
		XFontTexture* pTex = pFont->getFontTexture();
		if (pTex && pTex->IsReady()) 
		{
			X_LOG0("Fonts", "Name: ^2\"%s\"^7 Flags: [^1%s^7] Size: ^2(%" PRIi32 ",%" PRIi32 ")^7 Usage: ^2%" PRIi32 " ^7CacheMiss: ^2%" PRIi32,
				pFont->getName().c_str(), pFont->getFlags().ToString(FlagDesc),
				pTex->GetWidth(), pTex->GetHeight(), pTex->GetSlotUsage(), pTex->GetCacheMisses());
		}
		else 
		{
			X_LOG0("Fonts", "Name: ^2\"%s\"^7 Flags: [^1%s^7]", pFont->getName().c_str(), pFont->getFlags().ToString(FlagDesc));
		}
	}
	
	X_LOG0("Fonts", "------------- ^8Fonts End^7 --------------");
}

void XFontSystem::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
	X_UNUSED(jobSys);
#if 0
	Path<char> path(name);
	if (strUtil::IsEqual(".font", path.extension()))
	{
		path.removeExtension();

		XFFont* pFont = static_cast<XFFont*>(GetFont(path.fileName()));
		if (pFont)
		{
			pFont->Reload();
		}
		return true;
	}

	return false;
#else 
	X_UNUSED(name);
#endif
}

X_NAMESPACE_END
