#include "stdafx.h"
#include "SoundSys.h"

X_NAMESPACE_BEGIN(game)

namespace entity
{

	SoundSystem::SoundSystem()
	{
		pSound_ = gEnv->pSound;
	}

	bool SoundSystem::init(void)
	{


		return true;
	}



	void SoundSystem::update(core::FrameData& frame, EnitiyRegister& reg)
	{
		X_UNUSED(frame);

		auto view = reg.view<SoundObject, TransForm>();
		for (auto entity : view)
		{
			auto& snd = reg.get<SoundObject>(entity);
			auto trans = reg.get<TransForm>(entity);

			trans.pos += snd.offset;

			pSound_->setPosition(snd.handle, trans);
		}
	}


}

X_NAMESPACE_END