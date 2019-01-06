#pragma once

X_NAMESPACE_BEGIN(game)


struct NetInterpolationState
{
    NetInterpolationState() :
        frac(0.f),
        serverGameTimeMS(0),
        snapShotStartMS(0),
        snapShotEndMS(0)
    {
    }

    float frac;
    int32_t serverGameTimeMS;
    int32_t snapShotStartMS;
    int32_t snapShotEndMS;
};


X_NAMESPACE_END