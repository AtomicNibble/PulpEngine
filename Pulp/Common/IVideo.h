#pragma once



X_NAMESPACE_BEGIN(video)

static const char* VIDEO_FILE_EXTENSION = "ivf";

static const uint32_t VID_MAX_LOADED = 4;

struct IVideo
{
	virtual ~IVideo() {}


};


struct IVideoSys : public core::IEngineSysBase
{
	virtual ~IVideoSys() {}


	virtual IVideo* findVideo(const char* pVideoName) const X_ABSTRACT;
	virtual IVideo* loadVideo(const char* pVideoName) X_ABSTRACT;

	virtual void releaseVideo(IVideo* pVid) X_ABSTRACT;

	virtual bool waitForLoad(IVideo* pVideo) X_ABSTRACT; 
};


X_NAMESPACE_END