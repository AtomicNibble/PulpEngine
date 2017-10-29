#pragma once


X_NAMESPACE_BEGIN(video)


static const size_t IVF_FILE_HDR_SZ = 32;
static const size_t IVF_FRAME_HDR_SZ = 12;

struct IVFHdr
{
	static const uint32_t IVF_MAGIC = FOURCC('D', 'K', 'I', 'F');
	static const uint16_t IVF_VERSION = 0;

	uint32_t magic;		//	bytes 0-3    signature: 'DKIF'
	uint16_t version;	//	bytes 4-5    version (should be 0)
	uint16_t hdrSize;	//	bytes 6-7    length of header in bytes
	uint32_t codecFourCC;	//	bytes 8-11   codec FourCC (e.g., 'VP80')
	uint16_t width;		//	bytes 12-13  width in pixels
	uint16_t height;	//	bytes 14-15  height in pixels
	uint32_t frameRate;	//	bytes 16-19  frame rate
	uint32_t timeScale;	//	bytes 20-23  time scale
	uint32_t numFrames;	//	bytes 24-27  number of frames in file
	uint32_t unused;	//	bytes 28-31  unused

	X_INLINE bool isValid(void) const
	{
		if (magic != IVF_MAGIC) {
			X_ERROR("Video", "ivf magic is invalid. Magic: %" PRIu8 " Required: %" PRIu32, magic, IVF_MAGIC);
			return false;
		}

		if (version != IVF_VERSION) {
			X_ERROR("Video", "ivf version is invalid. FileVer: %" PRIu8 " RequiredVer: %" PRIu32, version, IVF_VERSION);
			return false;
		}

		return true;
	}
};

X_PACK_PUSH(4)
struct IVFFrameHdr
{
	uint32_t frameDataSize;
	uint64_t presentationTimestamp;
};
X_PACK_POP

X_ENSURE_SIZE(IVFHdr, IVF_FILE_HDR_SZ);
X_ENSURE_SIZE(IVFFrameHdr, IVF_FRAME_HDR_SZ);


X_NAMESPACE_END