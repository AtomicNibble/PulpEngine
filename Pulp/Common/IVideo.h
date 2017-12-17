#pragma once

#include <IRender.h>
#include <IAsyncLoad.h>


X_NAMESPACE_DECLARE(core,
	struct FrameTimeData;
)

X_NAMESPACE_BEGIN(video)

static const char* VIDEO_FILE_EXTENSION = "ivf";

static const uint32_t VID_MAX_LOADED = 4;

struct IVideo
{
	virtual ~IVideo() {}

	virtual render::TexID getTextureID(void) const X_ABSTRACT;

};


struct IVideoSys : public core::IEngineSysBase, public core::IAssetLoader
{

	virtual ~IVideoSys() {}

	virtual void update(const core::FrameTimeData& frameTimeInfo) X_ABSTRACT;

	virtual IVideo* findVideo(const char* pVideoName) const X_ABSTRACT;
	virtual IVideo* loadVideo(const char* pVideoName) X_ABSTRACT;

	virtual void releaseVideo(IVideo* pVid) X_ABSTRACT;
	virtual bool waitForLoad(IVideo* pVideo) X_ABSTRACT; 


	virtual void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const X_ABSTRACT;
};


X_NAMESPACE_END