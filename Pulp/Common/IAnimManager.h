#pragma once

#include <IAsyncLoad.h>

X_NAMESPACE_BEGIN(anim)

class Anim;

struct IAnimManager : public core::IAssetLoader
{
    using core::IAssetLoader::waitForLoad;

    virtual ~IAnimManager() = default;

    // returns null if not found, ref count unaffected
    virtual Anim* findAnim(core::string_view name) const X_ABSTRACT;
    virtual Anim* loadAnim(core::string_view name) X_ABSTRACT;

    virtual bool waitForLoad(Anim* pAnim) X_ABSTRACT; // returns true if load succeed.
};

X_NAMESPACE_END
