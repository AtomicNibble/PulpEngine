#include "stdafx.h"
#include "AnimManager.h"

#include <Assets\AssetLoader.h>

#include <IConsole.h>

#include <../../tools/AnimLib/AnimLib.h>

X_NAMESPACE_BEGIN(anim)

AnimManager::AnimManager(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena) :
    arena_(arena),
    blockArena_(blockArena),
    pAssetLoader_(nullptr),
    anims_(arena, sizeof(AnimResource), X_ALIGN_OF(AnimResource), "AnimPool")
{
}

AnimManager::~AnimManager()
{
}

void AnimManager::registerCmds(void)
{
    ADD_COMMAND_MEMBER("listAnims", this, AnimManager, &AnimManager::Cmd_ListAnims, core::VarFlag::SYSTEM, "List all the loaded anims");
} 

void AnimManager::registerVars(void)
{
}

bool AnimManager::init(void)
{
    pAssetLoader_ = gEnv->pCore->GetAssetLoader();
    pAssetLoader_->registerAssetType(assetDb::AssetType::ANIM, this, ANIM_FILE_EXTENSION);


    return true;
}

void AnimManager::shutDown(void)
{

    freeDangling();
}

bool AnimManager::asyncInitFinalize(void)
{
    return true;
}

Anim* AnimManager::findAnim(core::string_view name) const
{
    core::ScopedLock<AnimContainer::ThreadPolicy> lock(anims_.getThreadPolicy());

    AnimResource* pAnim = anims_.findAsset(name);
    if (pAnim) {
        return pAnim;
    }

    X_WARNING("AnimManager", "Failed to find anim: \"%*.s\"", name.length(), name.data());
    return nullptr;
}

Anim* AnimManager::loadAnim(core::string_view name)
{
    X_ASSERT(core::strUtil::FileExtension(name) == nullptr, "Extension not allowed")();

    core::ScopedLock<AnimContainer::ThreadPolicy> lock(anims_.getThreadPolicy());

    AnimResource* pAnimRes = anims_.findAsset(name);
    if (pAnimRes) {
        // inc ref count.
        pAnimRes->addReference();
        return pAnimRes;
    }

    // we create a anim and give it back
    core::string nameStr(name.data(), name.length());
    pAnimRes = anims_.createAsset(nameStr, nameStr, arena_);

    // add to list of anims that need loading.
    addLoadRequest(pAnimRes);

    return pAnimRes;
}

void AnimManager::releaseAnim(Anim* pAnim)
{
    AnimResource* pAnimRes = static_cast<AnimResource*>(pAnim);
    if (pAnimRes->removeReference() == 0) {
        releaseResources(pAnimRes);

        anims_.releaseAsset(pAnimRes);
    }
}


void AnimManager::listAnims(const char* pSearchPatten) const
{
    core::ScopedLock<AnimContainer::ThreadPolicy> lock(anims_.getThreadPolicy());

    core::Array<AnimResource*> sorted_anims(g_3dEngineArena);
    sorted_anims.setGranularity(anims_.size());

    for (const auto& anim : anims_) {
        auto* pAnimRes = anim.second;

        if (!pSearchPatten || core::strUtil::WildCompare(pSearchPatten, pAnimRes->getName())) {
            sorted_anims.push_back(pAnimRes);
        }
    }

    std::sort(sorted_anims.begin(), sorted_anims.end(), [](AnimResource* a, AnimResource* b) {
        const auto& nameA = a->getName();
        const auto& nameB = b->getName();
        return nameA.compareInt(nameB) < 0;
    });

    X_LOG0("Anim", "------------ ^8Anims(%" PRIuS ")^7 ---------------", sorted_anims.size());

    for (const auto* pAnim : sorted_anims) {
        X_LOG0("Anim", "^2%-32s^7 Dur: ^2%gms^7 Frames: ^2%" PRIi32 "^7 Bones: ^2%" PRIi32 "^7 Fps: ^2%" PRIi32 "^7 Notes: ^2%" PRIi32 " ^7Refs:^2%i",
            pAnim->getName().c_str(), pAnim->getDuration().GetMilliSeconds(),
            pAnim->getNumFrames(), pAnim->getNumBones(), pAnim->getFps(), pAnim->getNumNotes(), pAnim->getRefCount());
    }

    X_LOG0("Anim", "------------ ^8Anims End^7 --------------");
}

bool AnimManager::waitForLoad(core::AssetBase* pAnim)
{
    X_ASSERT(pAnim->getType() == assetDb::AssetType::ANIM, "Invalid asset passed")();

    if (pAnim->isLoaded()) {
        return true;
    }

    return waitForLoad(static_cast<Anim*>(pAnim));
}

bool AnimManager::waitForLoad(Anim* pAnim)
{
    if (pAnim->getStatus() == core::LoadStatus::Complete) {
        return true;
    }

    return pAssetLoader_->waitForLoad(pAnim);
}

// ------------------------------------

void AnimManager::freeDangling(void)
{
    {
        core::ScopedLock<AnimContainer::ThreadPolicy> lock(anims_.getThreadPolicy());

        // any left?
        for (const auto& a : anims_) {
            auto* pAnimRes = a.second;
            const auto& name = pAnimRes->getName();
            X_WARNING("Anim", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pAnimRes->getRefCount());

            releaseResources(pAnimRes);
        }
    }

    anims_.free();
}

void AnimManager::releaseResources(Anim* pAnim)
{
    X_UNUSED(pAnim);
}

// ------------------------------------

void AnimManager::addLoadRequest(AnimResource* pAnim)
{
    pAssetLoader_->addLoadRequest(pAnim);
}

void AnimManager::onLoadRequestFail(core::AssetBase* pAsset)
{
    auto* pAnim = static_cast<Anim*>(pAsset);

    X_UNUSED(pAnim);
}

bool AnimManager::processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize)
{
    auto* pAnim = static_cast<Anim*>(pAsset);

    return pAnim->processData(std::move(data), dataSize);
}

// -------------------------------------

bool AnimManager::onFileChanged(const core::AssetName& assetName, const core::string& name)
{
    X_UNUSED(assetName);

    core::ScopedLock<AnimContainer::ThreadPolicy> lock(anims_.getThreadPolicy());

    auto* pAnimRes = anims_.findAsset(name);
    if (!pAnimRes) {
        X_LOG1("Anim", "Not reloading \"%s\" it's not currently used", name.c_str());
        return false;
    }

    X_LOG0("Anim", "Reloading: %s", name.c_str());

    pAssetLoader_->reload(pAnimRes, core::ReloadFlag::Beginframe);
    return true;
}

// -------------------------------------

void AnimManager::Cmd_ListAnims(core::IConsoleCmdArgs* pCmd)
{
    // optional search criteria
    const char* pSearchPatten = nullptr;

    if (pCmd->GetArgCount() >= 2) {
        pSearchPatten = pCmd->GetArg(1);
    }

    listAnims(pSearchPatten);
}

X_NAMESPACE_END