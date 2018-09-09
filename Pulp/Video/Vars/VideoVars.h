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
    X_INLINE int32_t debug(void) const;

private:
    int32_t drawDebug_;
    int32_t debug_;
};

X_NAMESPACE_END

#include "VideoVars.inl"

