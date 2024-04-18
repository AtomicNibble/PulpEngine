#pragma once

#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Assets\AssertContainer.h>
#include <Containers\Fifo.h>
#include <Containers\PriorityQueue.h>
#include <Threading\Signal.h>

#include <ITexture.h>

#include "Vars\TextureVars.h"

X_NAMESPACE_DECLARE(core,
                    namespace V2 {
                        struct Job;
                        class JobSystem;
                    }

                    struct IoRequestBase;
                    struct XFileAsync;
                    struct IConsoleCmdArgs;)

X_NAMESPACE_DECLARE(texture,
    namespace CI {
        class XTexLoaderCI;
    })

X_NAMESPACE_BEGIN(engine)

class Texture;

class TextureManager : private core::IAssetLoadSink
{
    typedef core::AssetContainer<Texture, texture::TEX_MAX_LOADED_IMAGES, core::MultiThreadPolicy<core::Spinlock>> TextureContainer;
    typedef TextureContainer::Resource TextureResource;
    typedef TextureContainer::Resource TexRes;

    typedef std::array<Texture*, render::TextureSlot::ENUM_COUNT> TextureSlotArr;
    typedef core::FixedArray<texture::ITextureFmt*, 8> TextureLoadersArr;

    typedef core::MemoryArena<
        core::MallocFreeAllocator,
        core::MultiThreadPolicy<core::CriticalSection>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // X_ENABLE_MEMORY_DEBUG_POLICIES
        >
        BlockArena;

    struct StreamRequest
    {
        X_INLINE bool operator<(const StreamRequest& oth) const
        {
            return priority < oth.priority;
        }
        X_INLINE bool operator>(const StreamRequest& oth) const
        {
            return priority > oth.priority;
        }

        int32_t priority;
        Texture* pTexture;
    };

    typedef core::PriorityQueue<StreamRequest> StreamReqQueue;

public:
    TextureManager(core::MemoryArenaBase* arena);
    ~TextureManager();

    void registerCmds(void);
    void registerVars(void);

    bool init(void);
    void shutDown(void);

    bool asyncInitFinalize(void);
    void scheduleStreaming(void);


    Texture* findTexture(core::string_view name) const;
    Texture* loadTexture(core::string_view name, texture::TextureFlags flags);
    Texture* getDefault(render::TextureSlot::Enum slot) const;

    void releaseTexture(Texture* pTex);

    // returns true if load succeed.
    bool waitForLoad(core::AssetBase* pTexture);
    bool waitForLoad(Texture* pTexture);

private:

    bool loadDefaultTextures(void);
    void releaseDefaultTextures(void);
    void releaseDanglingTextures(void);
    void releaseResources(Texture* pTex);

private:
    void addLoadRequest(TextureResource* pTexture);
    void onLoadRequestFail(core::AssetBase* pAsset) X_FINAL;
    bool processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize) X_FINAL;
    bool onFileChanged(const core::AssetName& assetName, const core::string& name) X_FINAL;

    void listTextures(core::string_view searchPattern);

private:
    void Cmd_ListTextures(core::IConsoleCmdArgs* pCmd);

private:
    core::MemoryArenaBase* arena_;

    core::AssetLoader* pAssetLoader_;

    TextureVars vars_;
    TextureContainer textures_;

    texture::CI::XTexLoaderCI* pCILoader_;
    TextureLoadersArr textureLoaders_;

private:
    // streaming stuff
    BlockArena::AllocationPolicy blockAlloc_;
    BlockArena blockArena_;

    StreamReqQueue streamQueue_;

private:
    int32_t currentDeviceTexId_;

    Texture* pTexDefault_;
    Texture* pTexDefaultBump_;
    Texture* pTexDefaultWhite_;

    TextureSlotArr defaultLookup_;
};

X_NAMESPACE_END