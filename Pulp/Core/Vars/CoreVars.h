#pragma once

X_NAMESPACE_BEGIN(core)

struct ICVar;

class CoreVars
{
public:
    CoreVars();

    void registerVars(void);
    
    void setFullScreen(int32_t val);
    void updateWinPos(int32_t x, int32_t y);
    void updateWinDim(int32_t width, int32_t height);

    X_INLINE int32_t getSchedulerNumThreads(void) const;
    X_INLINE int32_t getCoreEventDebug(void) const;
    X_INLINE int32_t getCoreFastShutdown(void) const;

    X_INLINE int32_t getWinPosX(void) const;
    X_INLINE int32_t getWinPosY(void) const;
    X_INLINE int32_t getWinWidth(void) const;
    X_INLINE int32_t getWinHeight(void) const;
    X_INLINE int32_t getFullscreen(void) const;
    X_INLINE int32_t getWinVideoMode(void) const;

    X_INLINE core::ICVar* getVarWinPosX(void);
    X_INLINE core::ICVar* getVarWinPosY(void);
    X_INLINE core::ICVar* getVarWinWidth(void);
    X_INLINE core::ICVar* getVarWinHeight(void);
    X_INLINE core::ICVar* getVarWinCustomFrame(void);

private:
    int32_t schedulerNumThreads_;
    int32_t coreFastShutdown_;
    int32_t coreEventDebug_;

    int32_t winXPos_;
    int32_t winYPos_;
    int32_t winWidth_;
    int32_t winHeight_;

    int32_t fullscreen_;
    int32_t monitor_;
    int32_t videoMode_;

private:
    core::ICVar* pWinPosX_;
    core::ICVar* pWinPosY_;
    core::ICVar* pWinWidth_;
    core::ICVar* pWinHeight_;
    core::ICVar* pWinCustomFrame_;
};

X_NAMESPACE_END

#include "CoreVars.inl"