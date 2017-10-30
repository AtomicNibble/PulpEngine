#pragma once

#include <Assets\AssetBase.h>
#include <Threading\Signal.h>

#include <Containers\FixedByteStreamRing.h>

X_NAMESPACE_BEGIN(video)

struct IVFHdr;

struct FrameData
{
	size_t dataSize;
	const uint8_t* pData;
};


class Video : public core::AssetBase, public IVideo
{
	const int32_t IO_BUFFER_SIZE = 1024 * 64;
	const int32_t RING_BUFFER_SIZE = 1024 * 1024; // 1MB

	typedef core::Array<bool> BoolArr;
	typedef core::Array<uint8_t, core::ArrayAlignedAllocatorFixed<uint8_t, 64>, core::growStrat::Multiply> DataVec;


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

	void dispatchRead(void);
	
	bool decodeFrame(void);

private:
	void DecodeFrame_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:
	vpx_codec_ctx_t codec_;

	uint16_t width_;
	uint16_t height_;
	uint32_t frameRate_;
	uint32_t timeScale_;
	uint32_t numFrames_;


	// Loading stuff
	core::CriticalSection cs_;

	core::XFileAsync* pFile_;

	uint64_t fileOffset_;	// the file offset we last read from.
	uint64_t fileLength_;	// the total file length;

	core::FixedByteStreamOwning ringBuffer_; // buffer that hold pending IO data.

	DataVec ioBuffer_;		// file data read into here
	DataVec encodedFrame_;	// a frame that's about to be decoded is place here.
	DataVec decodedFrame_;	// a decoded frame, read for uploading to gpu.
};


X_NAMESPACE_END

#include "XVideo.inl"