#include "stdafx.h"
#include "Core.h"

#include <IConsole.h>
#include <IRender.h>
#include <IFont.h>
#include <I3DEngine.h>
#include <IGame.h>
#include <ISound.h>
#include <IScriptSys.h>



#include <IRenderAux.h>

#include <Threading\JobSystem2.h>
#include <Platform\Window.h>

X_USING_NAMESPACE;


bool XCore::RunGameLoop(void)
{
	while (PumpMessages())
	{
		X_PROFILE_BEGIN("GameLoop", core::ProfileSubSys::GAME);

		Update();
		RenderBegin();
		RenderEnd();
	}
	return true;
}

bool XCore::Update(void)
{
	X_PROFILE_BEGIN("CoreUpdate", core::ProfileSubSys::CORE);
	profileSys_.OnFrameBegin();
	time_.OnFrameBegin();

	if (env_.pJobSys) {
		env_.pJobSys->OnFrameBegin();
	}

	dirWatcher_.tick();

	if (env_.pGame)
	{
		env_.pGame->Update();
	}

	if (env_.p3DEngine)
		env_.p3DEngine->Update();

	if (env_.pInput)
	{
		env_.pInput->Update(true);
	}

	if (env_.pConsole)
	{
		// runs commands that have been deffered.
		env_.pConsole->OnFrameBegin();
	}

	if (env_.pSound)
	{
		// bum tis bum tis!
		env_.pSound->Update();
	}

	if (env_.pScriptSys)
	{
		env_.pScriptSys->Update();
	}


#if X_SUPER == 0 && 1
	static core::TimeVal start = time_.GetAsyncTime();
	core::TimeVal time = time_.GetAsyncTime();

	float val = time.GetDifferenceInSeconds(start);
	if (val >= 0.95f)
	{
		start = time;

		float fps = time_.GetFrameRate();
		float frametime = time_.GetFrameTime();

		core::StackString<128> title;
		title.clear();
		title.appendFmt(X_ENGINE_NAME " Engine " X_CPUSTRING " (fps:%i, %ims) Time: %I64u(x%g) UI: %I64u",
			static_cast<int>(fps),
			static_cast<int>(frametime * 1000.f),
			static_cast<__int64>(time_.GetFrameStartTime(core::ITimer::Timer::GAME).GetMilliSeconds()),
			time_.GetTimeScale(),
			static_cast<__int64>(time_.GetFrameStartTime(core::ITimer::Timer::UI).GetMilliSeconds())
		);

		pWindow_->SetTitle(title.c_str());
	}
#endif

	return true;
}



void XCore::RenderBegin(void)
{
	X_PROFILE_BEGIN("CoreRenderBegin", core::ProfileSubSys::CORE);


	env_.pRender->RenderBegin();
	env_.p3DEngine->OnFrameBegin();
}


void XCore::RenderEnd(void)
{
	{
		X_PROFILE_BEGIN("CoreRenderEnd", core::ProfileSubSys::CORE);

		if (render::IRenderAux* pAux = env_.pRender->GetIRenderAuxGeo()) {
			pAux->flush();
		}

		// draw me all the profile wins!
		profileSys_.Render();

		if (core::IConsole* pConsole = GetIConsole()) {
			pConsole->Draw();
		}
		
		env_.pRender->RenderEnd();
	}
	// End
	profileSys_.OnFrameEnd();


	bool enabled = var_profile->GetInteger() > 0;

	profileSys_.setEnabled(enabled);
	env_.profilerEnabled_ = enabled;
}
