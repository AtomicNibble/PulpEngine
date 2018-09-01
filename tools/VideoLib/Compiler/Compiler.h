#pragma once

#include <Containers\ByteStream.h>

X_NAMESPACE_DECLARE(core,
    struct XFile);

X_NAMESPACE_BEGIN(video)


typedef core::Array<uint8_t> DataArr;

struct Block
{
    Block(core::MemoryArenaBase* arena) :
        data(arena)
    {}

    TrackType::Enum type;
    bool isKey;
    int64_t timeNS;

    DataArr data;
};

// we have clusters for seeking.
struct Cluster
{
    typedef core::Array<Block> BlockArr;

    Cluster(core::MemoryArenaBase* arena) :
        blocks(arena)
    {}

    int64_t timeNS;
    int64_t durationNS;

    BlockArr blocks;
};


class VideoCompiler
{
public:
    typedef core::Array<uint8_t> DataVec;
    typedef core::Array<Cluster> ClusterArr;

public:
    VideoCompiler(core::MemoryArenaBase* arena);
    ~VideoCompiler();

    bool process(DataVec&& videoData);

    bool writeToFile(core::XFile* pFile) const;

private:
    core::MemoryArenaBase* arena_;

    int64_t durationNS_;

    VideoTrackHdr videoTrack_;
    AudioTrackHdr audioTrack_;

    DataArr audioHeaders_;
    ClusterArr clusters_;
};

X_NAMESPACE_END
