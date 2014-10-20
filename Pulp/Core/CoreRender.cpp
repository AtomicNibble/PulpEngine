#include "stdafx.h"
#include "Core.h"

#include <IConsole.h>
#include <IRender.h>
#include <IFont.h>
#include <I3DEngine.h>

X_USING_NAMESPACE;


void XCore::RenderBegin()
{
	X_PROFILE_BEGIN("CoreRenderBegin", core::ProfileSubSys::CORE);


	env_.pRender->RenderBegin();
	env_.p3DEngine->OnFrameBegin();
}


void XCore::RenderEnd()
{
	{
		X_PROFILE_BEGIN("CoreRenderEnd", core::ProfileSubSys::CORE);


		// draw me all the profile wins!
		if (var_profile_draw->GetInteger())
			profileSys_.Render();

		if (core::IConsole* pConsole = GetIConsole())
			pConsole->Draw();
	
		
		env_.pRender->RenderEnd();
	}
	// End
	profileSys_.FrameEnd();


	bool enabled = var_profile->GetInteger() > 0;

	profileSys_.setEnabled(enabled);
	env_.profilerEnabled_ = enabled;
}
