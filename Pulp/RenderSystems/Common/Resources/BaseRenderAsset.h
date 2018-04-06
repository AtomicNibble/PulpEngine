#pragma once

#ifndef X_BASE_RENDER_ASSET_H_
#define X_BASE_RENDER_ASSET_H_

#include <Assets\AssertContainer.h>

X_NAMESPACE_BEGIN(render)

class XRenderResourceContainer : public core::XResourceContainer
{
public:
    XRenderResourceContainer(core::MemoryArenaBase* arena, size_t size) :
        XResourceContainer(arena, size)
    {
    }

    virtual bool removeAsset(core::XBaseAsset* pAsset) X_OVERRIDE;
};

X_NAMESPACE_END

#endif // !X_BASE_RENDER_ASSET_H_