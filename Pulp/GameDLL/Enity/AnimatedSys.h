#pragma once


X_NAMESPACE_DECLARE(core,
	struct FrameTimeData;
)



X_NAMESPACE_BEGIN(game)

namespace entity
{

	class AnimatedSystem
	{
	public:
		AnimatedSystem();

		bool init(void);
		void update(core::FrameTimeData& time, EnitiyRegister& reg);


	private:

	};

}

X_NAMESPACE_END