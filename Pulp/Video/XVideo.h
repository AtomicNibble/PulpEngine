#pragma once


#include <Util\UniquePointer.h>
#include <Threading\Signal.h>

X_NAMESPACE_BEGIN(video)


/*
	I basically want to be able to load videos, stream them and decode into textures.
	
	I need to buffer the reads, as vpx, makes lots of single byte reads.
	I also would like the IO to be done with IO queue.

	so i basically need some sort of IO ring buffer that I dispatch requests to in order to 
	try and keep the buffer full.

	then my reader will just read from this buffer.
	annoyingly, the reader passes a offset, so hopfully it never try's to read backwards :S
	
	then we need a job that will decode a frame for displaying, guess we can just dispatch one each frame when needed.
	or maybe double buffer?

	then once we have a decoded frame needs uploading to the gpu.
	so we can render it, which needs to be a material really.


	so do i actually just want to build up decoded frame buffer rather than undecoded buffer.
	humm i think i'll stick to encoded buffer as it's smaller.

*/


typedef core::Array<uint8_t> DataVec;

class MkvReader : public mkvparser::IMkvReader
{
	const int32_t IO_REQUEST_GRAN = 1024 * 4;
	const int32_t BUFFER_SIZE = IO_REQUEST_GRAN * 256; // 4k * 64 = 1MB

	typedef core::Array<bool> BoolArr;

public:
	explicit MkvReader(core::XFileAsync* pFile, core::MemoryArenaBase* arena);
	~MkvReader() X_FINAL = default;

	int Read(long long offset, long len, unsigned char* buf) X_FINAL;
	int Length(long long* total, long long* available) X_FINAL;

	void SetBytesProccess(size_t numbytes);

private:
	int32_t avalibleBlocks(void) const;
	void fillBufffer();

	void IoCallback(core::IFileSys&, const core::IoRequestBase*, core::XFileAsync*, uint32_t);

protected:
	core::CriticalSection cs_;
	core::Signal signal_;
	BoolArr loadedBuffers_; // support the IO responses out of order.

	int64_t readFileOffset_; // the offset readBlock_ relates to in file.
	
	int32_t readBlock_;
	int32_t writeBlock_;
	int32_t totalBlocks_;
	DataVec buffer_;		// the ring buffer we read / write to/ multiple of request gran.


	uint64_t offset_; // the file offset we last read from.
	uint64_t length_; // the total file length;

	core::XFileAsync* pFile_;
};

struct VpxRational 
{
	VpxRational() {
		numerator = 0;
		denominator = 0;
	}

	int numerator;
	int denominator;
};


class XVideoWebm
{
public:
	XVideoWebm(core::XFileAsync* pFile, core::MemoryArenaBase* arena);

	bool parse();
	bool readFrame(void);

	const DataVec& getFrameData(void) const;

private:
	bool guessFramerate(void);

private:
	core::string fileName_;

	// webm
	MkvReader reader_;
	mkvparser::Segment* pSegment_;
	const mkvparser::Cluster* pCluster_;

	const mkvparser::Block* pBlock_;
	const mkvparser::BlockEntry* pBlockEntry_;

	int32_t videoTrackIndex_;
	int32_t blockFrameIndex_;

	uint64_t timestampNs_;
	int32_t reachedEos_;
	int32_t isKeyFrame_;

	DataVec buffer_;

	// vpx.
	int32_t width_;
	int32_t height_;
	VpxRational framerate_;

};


class XVideoSys : public IVideoSys
{
public:
	XVideoSys(core::MemoryArenaBase* arena);
	~XVideoSys() X_FINAL = default;

	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(void) X_FINAL;
	void shutDown(void) X_FINAL;
	void release(void) X_FINAL;


	void loadVideo(const char* pVideoName);


private:
	vpx_codec_ctx_t codec_;

};


X_NAMESPACE_END