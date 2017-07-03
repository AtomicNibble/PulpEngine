#pragma once

#ifndef X_GAME_H_
#define X_GAME_H_

#include <IGame.h>
#include <IInput.h>
#include <ITimer.h>

#include "Enity\EnitiySystem.h"


X_NAMESPACE_DECLARE(render,
	struct IRender
);
X_NAMESPACE_DECLARE(core,
	struct ICVar
);

X_NAMESPACE_BEGIN(game)

class XGame :
	public IGame,
	public input::IInputEventListner
{


public:
	XGame(ICore *pCore);
	~XGame() X_FINAL;

	// IGame
	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(void) X_FINAL;
	bool shutDown(void) X_FINAL;
	void release(void) X_FINAL;

	bool update(core::FrameData& frame) X_FINAL;
	// ~IGame

	// IInputEventListner
	bool OnInputEvent(const input::InputEvent& event) X_OVERRIDE;
	bool OnInputEventChar(const input::InputEvent& event) X_OVERRIDE;
	// ~IInputEventListner
		
private:
	void ProcessInput(core::FrameTimeData& timeInfo);
	
	void OnFovChanged(core::ICVar* pVar);

private:
	ICore* pCore_;

	render::IRender* pRender_;
	core::ITimer* pTimer_;
	core::ICVar* pFovVar_;

	Vec3f cameraPos_;
	Vec3f cameraAngle_;

	core::FixedArray<input::InputEvent, 256> inputEvents_;

	XCamera cam_;

private:
	EnititySystem ents_;
};

X_NAMESPACE_END

#endif // !X_GAME_H_