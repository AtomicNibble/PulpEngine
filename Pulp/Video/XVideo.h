#pragma once

#include <Assets\AssetBase.h>
#include <Threading\Signal.h>

X_NAMESPACE_BEGIN(video)

struct IVFHdr;

struct FrameData
{
	size_t dataSize;
	const uint8_t* pData;
};


class Video : public core::AssetBase, public IVideo
{
	// max io requests active, if we add request priority, could dispatch more with just with lower pri.
	// as the first ones should still complete sooner. 
	const int32_t IO_REQUEST_ACTIVE = 4;
	const int32_t IO_REQUEST_GRAN = 1024 * 64;
	const int32_t BUFFER_SIZE = IO_REQUEST_GRAN * 16; // 64k * 16 = 1MB

	typedef core::Array<bool> BoolArr;
	typedef core::Array<uint8_t> DataVec;


public:
	Video(core::string name, core::MemoryArenaBase* arena);
	virtual ~Video();

	bool processHdr(core::XFileAsync* pFile, const IVFHdr& hdr);

	bool getFrame(FrameData& frame) const;
	void discardFrame(FrameData& frame);

	X_INLINE uint16_t getWidth(void) const;
	X_INLINE uint16_t getHeight(void) const;
	X_INLINE uint32_t getFps(void) const;
	X_INLINE uint32_t getNumFrames(void) const;

private:

	void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

	int32_t avalibleBlocks(void) const;
	void dispatchReads(void);


private:
	vpx_codec_ctx_t codec_;

	uint16_t width_;
	uint16_t height_;
	uint32_t frameRate_;
	uint32_t timeScale_;
	uint32_t numFrames_;


	// Loading stuff
	core::CriticalSection cs_;
	core::Signal signal_;

	core::XFileAsync* pFile_;

	uint64_t fileOffset_;		// the file offset we last read from.
	uint64_t fileLength_;	// the total file length;

	int32_t readBlock_;
	int32_t writeBlock_;
	int32_t totalBlocks_;
	int32_t loadingBlocks_;

	BoolArr loadedBuffers_; // support the IO responses out of order.
	DataVec buffer_; // the ring buffer we read / write to
};


X_NAMESPACE_END

#include "XVideo.inl"