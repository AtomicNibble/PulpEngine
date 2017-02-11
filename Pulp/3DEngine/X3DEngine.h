#pragma once

#ifndef _X_RENDER_SYS_H_
#define _X_RENDER_SYS_H_

#include "I3DEngine.h"
#include "EngineBase.h"
#include "Level\Level.h"

#include <IModel.h>
#include <IRenderMesh.h>

#include "Gui\GuiManger.h"

#include "Drawing\PrimativeContext.h"

X_NAMESPACE_BEGIN(engine)





class X3DEngine : public I3DEngine, public XEngineBase, public core::IXHotReload
{
public:
	X3DEngine(core::MemoryArenaBase* arena);
	virtual ~X3DEngine() X_OVERRIDE;

	void registerVars(void) X_OVERRIDE;
	void registerCmds(void) X_OVERRIDE;

	bool Init(void) X_OVERRIDE;
	void ShutDown(void) X_OVERRIDE;
	void release(void) X_OVERRIDE;

	void Update(core::FrameData& frame) X_OVERRIDE;
	void OnFrameBegin(core::FrameData& frame) X_OVERRIDE;


	IPrimativeContext* getPrimContext(PrimContext::Enum user) X_OVERRIDE;
	IMaterialManager* getMaterialManager(void) X_OVERRIDE;

	// IXHotReload
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

private:

	void LoadMap(const char* mapName);
	void LoadDevMap(const char* mapName);


private:
	// vars / cmds
	void Command_Map(core::IConsoleCmdArgs* Cmd);
	void Command_DevMap(core::IConsoleCmdArgs* Cmd);

private:
	//~
	gui::XGuiManager guisMan_;

	level::Level level_;

	PrimativeContextSharedResources primResources_;
	PrimativeContext primContexts_[PrimContext::ENUM_COUNT];
};


X_NAMESPACE_END


#endif // !_X_RENDER_SYS_H_
