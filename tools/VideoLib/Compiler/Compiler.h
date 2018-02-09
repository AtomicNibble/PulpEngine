#pragma once


#include <Containers\ByteStream.h>

X_NAMESPACE_DECLARE(core,
	struct XFile;
);

X_NAMESPACE_BEGIN(video)

class VideoCompiler
{
	typedef core::Array<uint8_t> DataVec;

public:
	VideoCompiler(core::MemoryArenaBase* arena);
	~VideoCompiler();

	bool setVideo(DataVec&& videoData);

	bool writeToFile(core::XFile* pFile) const;

private:
	core::MemoryArenaBase* arena_;
	DataVec videoData_;
};


X_NAMESPACE_END