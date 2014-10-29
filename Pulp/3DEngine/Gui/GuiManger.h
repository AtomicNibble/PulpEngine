#pragma once

#ifndef X_GUI_MANAGER_H_
#define X_GUI_MANAGER_H_

#include <IGui.h>

X_NAMESPACE_BEGIN(gui)

class XGuiManager : public IGuiManger
{
public:
	XGuiManager();
	~XGuiManager() X_FINAL;

	void Init(void) X_FINAL;
	void Shutdown(void) X_FINAL;


private:


};


X_NAMESPACE_END

#endif // !X_GUI_MANAGER_H_