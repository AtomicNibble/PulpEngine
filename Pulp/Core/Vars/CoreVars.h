#pragma once


X_NAMESPACE_BEGIN(core)

struct ICVar;

class CoreVars
{
public:
	CoreVars();

	void registerVars(void);

	void updateWinPos(int32_t x, int32_t y);
	void updateWinDim(int32_t width, int32_t height);

	

public:
	int32_t schedulerNumThreads_;
	int32_t coreFastShutdown_;
	int32_t coreEventDebug_;

	int32_t winXPos_;
	int32_t winYPos_;
	int32_t winWidth_;
	int32_t winHeight_;

private:
	core::ICVar* pWinPosX_;
	core::ICVar* pWinPosY_;
	core::ICVar* pWinWidth_;
	core::ICVar* pWinHeight_;
	core::ICVar* pWinCustomFrame_;
};


X_NAMESPACE_END