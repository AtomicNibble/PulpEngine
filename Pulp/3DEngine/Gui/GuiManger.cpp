#include "stdafx.h"
#include "GuiManger.h"

#include "EngineBase.h"

#include <IConsole.h>
#include <IRender.h>

#include <algorithm>


X_NAMESPACE_BEGIN(gui)                                                                                                                                        

namespace
{


static void sortGuisByName(core::Array<XGui*>& vars)
	{
		using namespace std;

		std::sort(vars.begin(), vars.end(),
			[](XGui* a, XGui* b){
				return ::strcmp(a->getName(), b->getName()) < 0;
			}
		);
	}


}

void Command_ListUis(core::IConsoleCmdArgs* pArgs)
{
	// we support wildcards
	const char* pSearchString = nullptr;
	if (pArgs->GetArgCount() > 0)
	{
		pSearchString = pArgs->GetArg(1);
	}


	engine::XEngineBase::getGuiManager()->listGuis();
}


XGuiManager::XGuiManager() :
	guis_(g_3dEngineArena),
	pCursorArrow_(nullptr)
{
	screenRect_.set(0, 0, 800, 600);

	// meh
	guis_.reserve(GUI_MAX_MENUS);
}

XGuiManager::~XGuiManager()
{

}


bool XGuiManager::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pCore);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);
	X_ASSERT_NOT_NULL(gEnv->pInput);
	X_ASSERT_NOT_NULL(gEnv->pRender);
	X_LOG0("Gui", "Starting GUI System");

	ADD_COMMAND("ui_list", Command_ListUis, 0, "List the loaded ui's");

	ADD_CVAR_REF("ui_DrawDebug", var_showDebug_, 1, 0, 1, core::VarFlag::SYSTEM, "draw debug info over gui");

	gEnv->pHotReload->addfileType(this, gui::GUI_FILE_EXTENSION);
	gEnv->pHotReload->addfileType(this, gui::GUI_BINARY_FILE_EXTENSION);
	// what's a gui without input :| ?
	gEnv->pInput->AddEventListener(this);

	// what you pointing at? rude..
	pCursorArrow_ = gEnv->pRender->LoadTexture("core_assets/Textures/cursor_arrow.dds",
		texture::TextureFlags::DONT_STREAM | texture::TextureFlags::NOMIPS);

	if (!pCursorArrow_) {
		X_FATAL("Gui", "failed to load main cursor");
		return false;
	}

	return true;
}

void XGuiManager::Shutdown(void)
{
	X_LOG0("Gui", "Shuting down GUI Systems");


	gEnv->pHotReload->addfileType(nullptr, gui::GUI_FILE_EXTENSION);
	gEnv->pHotReload->addfileType(nullptr, gui::GUI_BINARY_FILE_EXTENSION);

	gEnv->pInput->RemoveEventListener(this);

	if (pCursorArrow_)
		pCursorArrow_->release();
}

IGui* XGuiManager::loadGui(const char* name)
{
	X_ASSERT_NOT_NULL(name);
	XGui* pGui;

	if (pGui = static_cast<XGui*>(findGui(name)))
		return pGui;

	// try load it :|
	pGui = X_NEW(XGui, g_3dEngineArena, "GuiInterface");

	if (pGui->InitFromFile(name))
	{
		guis_.append(pGui);
		return pGui;
	}

	X_DELETE(pGui, g_3dEngineArena);
	return nullptr;
}

IGui* XGuiManager::findGui(const char* name)
{
	X_ASSERT_NOT_NULL(name);

	Guis::ConstIterator it;

	const char* nameBegin = name;
	const char* nameEnd = name + core::strUtil::strlen(name);

	for (it = guis_.begin(); it != guis_.end(); ++it)
	{
		if (core::strUtil::IsEqual(nameBegin, nameEnd, (*it)->getName()))
		{
			return (*it);
		}
	}

	return nullptr;
}




void XGuiManager::listGuis(const char* wildcardSearch) const
{
	// sort them.
	Guis::ConstIterator itrGui, itrGuiEnd = guis_.end();
	Guis sorted_guis(g_3dEngineArena);
	sorted_guis.reserve(guis_.size());

	for (itrGui = guis_.begin(); itrGui != itrGuiEnd; ++itrGui)
	{
		XGui* pGui = *itrGui;

		if (!wildcardSearch || core::strUtil::WildCompare(wildcardSearch, pGui->getName()))
		{
			sorted_guis.append(pGui);
		}
	}

	sortGuisByName(sorted_guis);
	X_LOG0("Gui", "-------------- ^8Guis(%i)^7 ---------------", sorted_guis.size());
	X_LOG_BULLET;

	itrGui = sorted_guis.begin();
	for (; itrGui != sorted_guis.end(); ++itrGui)
	{
		const XGui* pGui = *itrGui;
		X_LOG0("Gui", "^2\"%s\"", pGui->getName());
	}

	X_LOG0("Gui", "-------------- ^8Guis End^7 --------------");
}


// IXHotReload
bool XGuiManager::OnFileChange(const char* name)
{
	core::Path path(name);
	XGui* pGui;

	// we don't keep extension for name.
	path.removeExtension();

	if (pGui = static_cast<XGui*>(findGui(path.fileName())))
	{
		path = path.fileName();

		X_LOG0("Gui", "reloading \"%s\"", path.c_str());

		if (pGui->InitFromFile(path.fileName()))
		{

		}
		else
		{
			X_ERROR("Gui", "reload failed");
		}
	}
	else
	{
		X_LOG0("Gui", "menu \"%s\" is not used, skipping reload.", name);
	}

	return false;
}
// ~IXHotReload

// IInputEventListner
bool XGuiManager::OnInputEvent(const input::InputEvent& event)
{
	Guis::Iterator it;
	for (it = guis_.begin(); it != guis_.end(); ++it) {
		if ((*it)->OnInputEvent(event))
			return true;
	}
	return false;
}

bool XGuiManager::OnInputEventChar(const input::InputEvent& event)
{
	Guis::Iterator it;
	for (it = guis_.begin(); it != guis_.end(); ++it) {
		if ((*it)->OnInputEventChar(event))
			return true;
	}
	return false;
}
// ~IInputEventListner

X_NAMESPACE_END