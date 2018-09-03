#pragma once

#include <Util\UniquePointer.h>
#include <Threading\Signal.h>

#include <Assets\AssertContainer.h>

#include "Vars\VideoVars.h"

X_NAMESPACE_DECLARE(core,
                    namespace V2 {
                        struct Job;
                        class JobSystem;
                    }

                    struct XFileAsync;
                    struct IoRequestBase;
                    struct IConsoleCmdArgs;)

X_NAMESPACE_BEGIN(video)

/*
	I basically want to be able to load videos, stream them and decode into textures.

	I want to buffer reads for the video.
	a ring buffer split up into blocks.

	i'm going to use ivf so it's simple to read,
	each frame is after the next.

	when we do the buffered reads we want to pull out complete frames.
	that just means might have to wait for multiple IO buffers to fill, which is fine.

	just need decent logic for working out when we can move read pointer forward.
	basically just need to set bytes read.

	the problem i have is i kinda want to load the header, and mark the video as loaded.
	since then we would hav meta about the video.

	so how do we get the header?
	can we make it so can read out the header from buffered reader?

	or do we want to read the header standalone, then move into buffered reader.
	i mean we will still have to wait and read the frame headers just the same.

	so should we make videos behave like other assets like textures.
	since they need to be streamed also.

	we ask for a video file, then need to wait for it to be loaded before we can started decoding / rendering it.
	been loaded is just waiting for the header.

	who will decode a frame?

	we will need to dispatch frame decodes at start of frame?

*/


#if 0

class BufferedReader 
{
	const int32_t IO_REQUEST_GRAN = 1024 * 4;
	const int32_t BUFFER_SIZE = IO_REQUEST_GRAN * 256; // 4k * 64 = 1MB

	typedef core::Array<bool> BoolArr;

public:
	BufferedReader(core::XFileAsync* pFile, core::MemoryArenaBase* arena);
	~BufferedReader() = default;

	const uint8_t* getBytes(size_t size);
	void setBytesRead(size_t size);

private:
	int32_t avalibleBlocks(void) const;
	void fillBufffer();

	void IoCallback(core::IFileSys&, const core::IoRequestBase*, core::XFileAsync*, uint32_t);

protected:
	core::CriticalSection cs_;
	core::Signal signal_;
	BoolArr loadedBuffers_; // support the IO responses out of order.

	int64_t readBlockOffset_; // the offset from currect readblock.
	
	int32_t readBlock_;
	int32_t writeBlock_;
	int32_t totalBlocks_;
	DataVec buffer_;		// the ring buffer we read / write to/ multiple of request gran.


	uint64_t offset_; // the file offset we last read from.
	uint64_t length_; // the total file length;

	core::XFileAsync* pFile_;
};

#endif

typedef core::Array<uint8_t> DataVec;

class Video;

// Problem with using assetLoader for this is that i need to keep the file handle open after.
// and continue to make additonal requests.
struct VideoLoadRequest
{
    VideoLoadRequest(core::MemoryArenaBase* arena, Video* pVideo) :
        pFile(nullptr),
        pVideo(pVideo),
        buffer(arena)
    {
    }

    core::XFileAsync* pFile;
    Video* pVideo;
    DataVec buffer;
};

class XVideoSys : public IVideoSys
{
    typedef core::AssetContainer<Video, VIDEO_MAX_LOADED, core::SingleThreadPolicy> VideoContainer;
    typedef VideoContainer::Resource VideoResource;

    typedef core::Array<VideoLoadRequest*> VideoLoadRequestArr;

public:
    XVideoSys(core::MemoryArenaBase* arena);
    ~XVideoSys() X_FINAL = default;

    void registerVars(void) X_FINAL;
    void registerCmds(void) X_FINAL;

    bool init(void) X_FINAL;
    void shutDown(void) X_FINAL;
    void release(void) X_FINAL;

    void update(const core::FrameTimeData& frameTimeInfo) X_FINAL;

    void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const X_FINAL;

    IVideo* findVideo(const char* pVideoName) const X_FINAL;
    IVideo* loadVideo(const char* pVideoName) X_FINAL;

    void releaseVideo(IVideo* pVid) X_FINAL;
    bool waitForLoad(core::AssetBase* pVideo) X_FINAL; // returns true if load succeed.
    bool waitForLoad(IVideo* pVideo) X_FINAL;          // returns true if load succeed.

    void listVideos(const char* pSearchPatten = nullptr) const;

private:
    void freeDangling(void);

private:
    void queueLoadRequest(VideoResource* pVideoRes);
    void dispatchLoadRequest(VideoLoadRequest* pLoadReq);

    void onLoadRequestFail(VideoLoadRequest* pLoadReq);
    void loadRequestCleanup(VideoLoadRequest* pLoadReq);

    void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
        core::XFileAsync* pFile, uint32_t bytesTransferred);

    void processData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:
    void Cmd_ListVideo(core::IConsoleCmdArgs* pCmd);

private:
    core::MemoryArenaBase* arena_;
    core::MemoryArenaBase* blockArena_; // for video buffers

    VideoVars vars_;

    // loading
    core::CriticalSection loadReqLock_;
    core::ConditionVariable loadCond_;

    VideoContainer videos_;
    VideoLoadRequestArr pendingRequests_;
};

X_NAMESPACE_END