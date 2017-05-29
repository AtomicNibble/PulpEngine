#pragma once

#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Assets\AssertContainer.h>
#include <Containers\Fifo.h>
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
)

X_NAMESPACE_DECLARE(texture,
	namespace CI {
		class XTexLoaderCI;
	}	
)

X_NAMESPACE_BEGIN(engine)

class Texture;

class TextureManager : public core::IXHotReload
{
	typedef core::AssetContainer<Texture, texture::TEX_MAX_LOADED_IMAGES, core::MultiThreadPolicy<core::Spinlock>> TextureContainer;
	typedef TextureContainer::Resource TextureResource;
	typedef TextureContainer::Resource TexRes;

	typedef std::array<Texture*, render::TextureSlot::ENUM_COUNT> TextureSlotArr;
	typedef core::FixedArray<texture::ITextureFmt*, 8> TextureLoadersArr;
	typedef core::Fifo<Texture*> TextureQueue;

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
#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES
	> BlockArena;

public:
	TextureManager(core::MemoryArenaBase* arena);
	~TextureManager();

	void registerCmds(void);
	void registerVars(void);
	
	bool init(void);
	void shutDown(void);

	bool asyncInitFinalize(void);

	void scheduleStreaming(void);


	Texture* forName(const char* pName, texture::TextureFlags flags);
	Texture* getDefault(render::TextureSlot::Enum slot) const;

	void releaseTexture(Texture* pTex);

private:
	void dispatchRead(Texture* pTexture);

//	bool loadFromFile(texture::XTextureFile& imgFile, const char* pPath);
//	bool processImgFile(Texture* pTex, texture::XTextureFile& imgFile);

	bool loadDefaultTextures(void);
	void releaseDefaultTextures(void);
	void releaseDanglingTextures(void);

private:
	void IoRequestCallback(core::IFileSys&, const core::IoRequestBase*, core::XFileAsync*, uint32_t);
	
	void processCIImageData(Texture* pTexture, const uint8_t* pData, size_t length);

	// IXHotReload
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

private:
	core::MemoryArenaBase* arena_;
	TextureVars vars_;
	TextureContainer textures_;

	texture::CI::XTexLoaderCI* pCILoader_;
	TextureLoadersArr textureLoaders_;

private:
	// streaming stuff
	BlockArena::AllocationPolicy blockAlloc_;
	BlockArena blockArena_;

	TextureQueue streamQueue_;

	core::Signal loadComplete_;

private:
	Texture* pTexDefault_;
	Texture* pTexDefaultBump_;
	
	TextureSlotArr defaultLookup_;
};



X_NAMESPACE_END