#pragma once

#ifndef X_GUI_MANAGER_H_
#define X_GUI_MANAGER_H_

#include <IGui.h>
#include <IInput.h>

#include "Gui.h"

#include <Containers\Array.h>

X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
);

X_NAMESPACE_BEGIN(gui)

class XGuiManager :
	public IGuiManger, 
	public core::IXHotReload,
	public input::IInputEventListner
{
public:
	XGuiManager();
	~XGuiManager() X_FINAL;

	//IGuiManger
	bool Init(void) X_FINAL;
	void Shutdown(void) X_FINAL;

	IGui* loadGui(const char* name) X_FINAL;
	IGui* findGui(const char* name) X_FINAL;

	void listGuis(const char* wildcardSearch = nullptr) const X_FINAL;
	//~IGuiManger

	// IXHotReload
	bool OnFileChange(const char* name) X_FINAL;
	// ~IXHotReload

	// IInputEventListner
	bool OnInputEvent(const input::InputEvent& event) X_FINAL;
	bool OnInputEventChar(const input::InputEvent& event) X_FINAL;
	// ~IInputEventListner

	X_INLINE bool ShowDeubug(void) const{
		return var_showDebug_ == 1;
	}

private:
	typedef core::Array<XGui*> Guis;

	Rectf screenRect_;
	Guis guis_;

	int var_showDebug_;

	texture::ITexture* pCursorArrow_;


	friend void Command_ListUis(core::IConsoleCmdArgs* pArgs);
};


X_NAMESPACE_END

#endif // !X_GUI_MANAGER_H_