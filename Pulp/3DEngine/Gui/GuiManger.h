#pragma once

#ifndef X_GUI_MANAGER_H_
#define X_GUI_MANAGER_H_

#include <IGui.h>

#include "Gui.h"

#include <Containers\Array.h>

X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
);

X_NAMESPACE_BEGIN(gui)

class XGuiManager : public IGuiManger, public core::IXHotReload
{
public:
	XGuiManager();
	~XGuiManager() X_FINAL;

	//IGuiManger
	void Init(void) X_FINAL;
	void Shutdown(void) X_FINAL;

	IGui* loadGui(const char* name) X_FINAL;
	IGui* findGui(const char* name) X_FINAL;

	void listGuis(const char* wildcardSearch = nullptr) const X_FINAL;
	//~IGuiManger

	// IXHotReload
	bool OnFileChange(const char* name) X_FINAL;
	// ~IXHotReload

private:
	typedef core::Array<XGui*> Guis;

	Rectf screenRect_;

	Guis guis_;


	friend void Command_ListUis(core::IConsoleCmdArgs* pArgs);
};


X_NAMESPACE_END

#endif // !X_GUI_MANAGER_H_