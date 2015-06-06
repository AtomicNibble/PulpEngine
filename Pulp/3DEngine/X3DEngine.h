#pragma once

#ifndef _X_RENDER_SYS_H_
#define _X_RENDER_SYS_H_

#include "I3DEngine.h"
#include "EngineBase.h"
#include "Level.h"

#include <IModel.h>
#include <IRenderMesh.h>

#include "Gui\GuiManger.h"

X_NAMESPACE_BEGIN(engine)





struct X3DEngine : public I3DEngine, public XEngineBase, public core::IXHotReload
{

	virtual bool Init(void) X_OVERRIDE;
	virtual void ShutDown(void) X_OVERRIDE;
	virtual int release(void) X_OVERRIDE;

	virtual void OnFrameBegin(void) X_OVERRIDE;
	virtual void Update(void) X_OVERRIDE;


	// IXHotReload
	virtual bool OnFileChange(const char* name) X_FINAL;
	// ~IXHotReload

private:
	void RegisterCmds(void);

	void LoadMap(const char* mapName);
	void LoadDevMap(const char* mapName);


private:
	// vars / cmds
	friend void Command_Map(core::IConsoleCmdArgs* Cmd);
	friend void Command_DevMap(core::IConsoleCmdArgs* Cmd);


	//~

	gui::XGuiManager guisMan_;

	level::Level level_;


	model::XModel model, modelSky;
	model::IRenderMesh* pMesh;
	model::IRenderMesh* pSkybox;
};


X_NAMESPACE_END


#endif // !_X_RENDER_SYS_H_
