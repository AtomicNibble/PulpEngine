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
#include <IFrameData.h>

#include <Threading\JobSystem2.h>
#include <Platform\Window.h>

X_USING_NAMESPACE;


bool XCore::RunGameLoop(void)
{
#if X_ENABLE_ASSERTIONS
	env_.initComplete_ = true;
#endif // !X_ENABLE_ASSERTIONS

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

#if 1

	// we want to do everything via jobs. :D 
	// we also want frame data.
	core::FrameData frameData;
	frameData.startTime = time_.GetFrameStartTime();
	frameData.delta = time_.GetFrameTime();
	frameData.flags.Set(core::FrameFlag::HAS_FOCUS);

	// we have a load of things todo.
	/*
		Get input. (input events)
		Check dir watcher for changed files.
		Console?
		3dEngine:
			animations
			loading of assets
			culling
			greating drawcalls.
		sounds
		scripts sys
		physics

	
		Do we want to be fancy and depending on how much spare frame tiem we have for the target fps, allow more streaming jobs to happen.

	*/


	core::V2::JobSystem& jobSys = *env_.pJobSys;

	// top job that we can use to wait for the chain of jobs to complete.
	core::V2::Job* pSyncJob = jobSys.CreateJob(&core::V2::JobSystem::EmptyJob, nullptr);

	// start a job to handler any file chnages and create relaod child jobs.
	jobSys.AddContinuation(pSyncJob, jobSys.CreateMemberJobAsChild<XCore>(pSyncJob, this, &XCore::Job_DirectoryWatcher, nullptr));
	jobSys.AddContinuation(pSyncJob, jobSys.CreateMemberJobAsChild<XCore>(pSyncJob, this, &XCore::Job_ProcessInput, &frameData));

	jobSys.Run(pSyncJob);
	jobSys.Wait(pSyncJob);



	if (env_.pGame) {
		env_.pGame->Update();
	}

	if (env_.p3DEngine) {
		env_.p3DEngine->Update();
	}

	if (env_.pConsole) {
//		env_.pConsole->OnFrameBegin();
	}

	if (env_.pSound) {
		env_.pSound->Update();
	}

	if (env_.pScriptSys){
		env_.pScriptSys->Update();
	}


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

	int goat = 0;
	goat = 2;

#else

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
			pConsole->draw();
		}
		
		env_.pRender->RenderEnd();
	}
	// End
	profileSys_.OnFrameEnd();


	bool enabled = var_profile->GetInteger() > 0;

	profileSys_.setEnabled(enabled);
	env_.profilerEnabled_ = enabled;
}
