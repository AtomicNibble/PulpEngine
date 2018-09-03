#pragma once


X_NAMESPACE_DECLARE(core, struct ICVar)

X_NAMESPACE_BEGIN(video)

class VideoVars
{
public:
    VideoVars();
    ~VideoVars();

    void RegisterVars(void);

    X_INLINE bool drawDebug(void) const;

private:
    int32_t drawDebug_;
};

X_NAMESPACE_END

#include "VideoVars.inl"

