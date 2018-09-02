#include "stdafx.h"
#include "Compiler.h"

#include <IFileSys.h>

#include <Compression\LZ4.h>

X_NAMESPACE_BEGIN(video)

namespace
{

    class MKVBufferReader : public mkvparser::IMkvReader
    {
        typedef VideoCompiler::DataVec DataVec;

    public:
        MKVBufferReader(const DataVec& data) :
            data_(data)
        {}
        ~MKVBufferReader() X_FINAL = default;

        int Read(long long offset, long len, unsigned char* pBuf) X_FINAL {

            if (offset < 0) {
                return -1;
            }

            if (len < 0) {
                return -1;
            }

            if (len == 0) {
                return 0;
            }

            if (offset >= static_cast<long long>(data_.size())) {
                return -1;
            }

            std::memcpy(pBuf, data_.data() + offset, len);
            return 0;
        }

        int Length(long long* pTotal, long long* pAvailable) X_FINAL {
            if (pTotal) {
                *pTotal = data_.size();
            }
            if (pAvailable) {
                *pAvailable = data_.size();
            }
            return 0;
        }

    private:
        const DataVec& data_;
    };

} // namespace


VideoCompiler::VideoCompiler(core::MemoryArenaBase* arena) :
    arena_(arena),
    durationNS_(0),
    audioHeaders_(arena),
    clusters_(arena)
{
    core::zero_object(videoTrack_);
    core::zero_object(audioTrack_);
}

VideoCompiler::~VideoCompiler()
{
}

bool VideoCompiler::process(DataVec&& srcData)
{
    // load a webm..
    MKVBufferReader reader(srcData);

    auto ebmlHeader = std::make_unique<mkvparser::EBMLHeader>();

    int64_t pos = 0;

    if (ebmlHeader->Parse(&reader, pos) < 0) {
        X_ERROR("Video", "Failed to parse header");
        return false;
    }

    mkvparser::Segment* pSegmentTmp;
    if (mkvparser::Segment::CreateInstance(&reader, pos, pSegmentTmp)) {
        X_ERROR("Video", "Failed to create segment");
        return false;
    }

    std::unique_ptr<mkvparser::Segment> segment(pSegmentTmp);
    if (segment->Load() < 0) {
        X_ERROR("Video", "Failed to load segment");
        return false;
    }

    auto* pTracks = segment->GetTracks();
    if (!pTracks) {
        X_ERROR("Video", "Failed to get tracks");
        return false;
    }

    auto* pSegmentInfo = segment->GetInfo();
    if (!pSegmentInfo) {
        X_ERROR("Video", "Failed to get segment info");
        return false;
    }

    // for now just one video and audio track.
    auto numTrackes = pTracks->GetTracksCount();

    const mkvparser::VideoTrack* pVideoTrack = nullptr;
    const mkvparser::AudioTrack* pAudioTrack = nullptr;

    for (uint32_t i = 0; i < numTrackes; i++)
    {
        auto* pTrack = pTracks->GetTrackByIndex(i);

        auto type = pTrack->GetType();
        const char* pCodecID = pTrack->GetCodecId();

        if (type == mkvparser::Track::kVideo)
        {
            if (!core::strUtil::IsEqual(pCodecID, "V_VP8"))
            {
                X_ERROR("Video", "Unsupported video codec: %s", pCodecID);
                return false;
            }

            pVideoTrack = static_cast<const mkvparser::VideoTrack*>(pTrack);
        }
        else if (type == mkvparser::Track::kAudio)
        {
            if (!core::strUtil::IsEqual(pCodecID, "A_VORBIS"))
            {
                X_ERROR("Video", "Unsupported audio codec: %s", pCodecID);
                return false;
            }

            pAudioTrack = static_cast<const mkvparser::AudioTrack*>(pTrack);
        }
        else
        {
            continue;
        }
    }

    X_ASSERT_NOT_NULL(pVideoTrack);
    X_ASSERT_NOT_NULL(pAudioTrack);

    durationNS_ = pSegmentInfo->GetDuration();

    core::zero_object(videoTrack_);
    videoTrack_.pixelWidth = safe_static_cast<int32_t>(pVideoTrack->GetWidth());
    videoTrack_.pixelHeight = safe_static_cast<int32_t>(pVideoTrack->GetHeight());

    audioTrack_.channels = safe_static_cast<int16_t>(pAudioTrack->GetChannels());
    audioTrack_.bitDepth = safe_static_cast<int16_t>(pAudioTrack->GetBitDepth());
    audioTrack_.sampleFreq = safe_static_cast<int32_t>(pAudioTrack->GetSamplingRate());

    if (audioTrack_.channels > VIDEO_MAX_AUDIO_CHANNELS) {
        X_ERROR("Video", "Audio has unsupported channel count: %" PRIi32, audioTrack_.channels);
        return false;
    }

    // load the vorbis headers.
    {
        size_t vorbisHdrSize = 0;
        const uint8_t* pData = pAudioTrack->GetCodecPrivate(vorbisHdrSize);

        // we need this, otherwise can't decode.
        if (vorbisHdrSize <= 0) {
            X_ERROR("Video", "Vorbis header data emptpy");
            return false;
        }

        audioHeaders_.resize(vorbisHdrSize);
        std::memcpy(audioHeaders_.data(), pData, vorbisHdrSize);
    }

    auto numClusters = segment->GetCount();
    const mkvparser::Cluster* pCluster = segment->GetFirst();

    // TODO: maybe support more audio tracks.
    std::array<std::pair<int32_t, TrackType::Enum>, 2> trackTypeLookup;
    trackTypeLookup[0] = std::make_pair(pVideoTrack->GetNumber(), TrackType::Video);
    trackTypeLookup[1] = std::make_pair(pAudioTrack->GetNumber(), TrackType::Audio);

    clusters_.clear();
    clusters_.reserve(numClusters);
    clusters_.setGranularity(numClusters);

    while (pCluster != nullptr && !pCluster->EOS())
    {
        auto timeNS = pCluster->GetTime();
        auto durationNS = pCluster->GetLastTime() - pCluster->GetFirstTime();
        auto numBlocks = pCluster->GetEntryCount();

        {
            const mkvparser::BlockEntry* pBlockEntry;
            auto status = pCluster->GetFirst(pBlockEntry);

            if (status) {
                X_ERROR("Video", "Could not get first Block of Cluster");
                return false;
            }

            auto& cluster = clusters_.AddOne(arena_);
            cluster.timeNS = timeNS;
            cluster.durationNS = durationNS;
            cluster.blocks.reserve(numBlocks);

            while (pBlockEntry != nullptr && !pBlockEntry->EOS())
            {
                if (pBlockEntry->GetKind() != mkvparser::BlockEntry::kBlockSimple) {
                    X_ERROR("Video", "Could not get first Block of Cluster");
                    return false;
                }

                auto& block = cluster.blocks.AddOne(arena_);

                auto* pBlock = pBlockEntry->GetBlock();
                auto trackNumber = pBlock->GetTrackNumber();
                auto numFrames = pBlock->GetFrameCount();
                if (numFrames > 1) {
                    X_ERROR("Video", "Block has multiple frames, no supported");
                    return false;
                }

                int32_t i;
                for (i = 0; i < static_cast<int32_t>(trackTypeLookup.size()); i++) {
                    if (trackTypeLookup[i].first == trackNumber) {
                        block.type = trackTypeLookup[i].second;
                        break; // not breaking probs faster lol. Unroll me \o/
                    }
                }

                if (i >= static_cast<int32_t>(trackTypeLookup.size())) {
                    X_ERROR("Video", "Failed to map track number %" PRIi64 " to type", trackNumber);
                    return false;
                }

                block.isKey = pBlock->IsKey();
                block.timeNS = pBlock->GetTime(pCluster);

                for (i = 0; i < numFrames; i++)
                {
                    const auto& frame = pBlock->GetFrame(i);
                    block.data.resize(frame.len);

                    if (frame.Read(&reader, block.data.data()) < 0) {
                        X_ERROR("Video", "Failed to read block frame data");
                        return false;
                    }

                    if (block.type == TrackType::Video) {
                        videoTrack_.numFrames++;
                        videoTrack_.largestFrameBytes = core::Max(videoTrack_.largestFrameBytes,
                            safe_static_cast<int32_t>(block.data.size()));
                    }
                }

                status = pCluster->GetNext(pBlockEntry, pBlockEntry);
                if (status) {
                    X_ERROR("Video", "Could not get next block of cluster");
                    return false;
                }
            }
        }
        pCluster = segment->GetNext(pCluster);
    }

    // validate vorbis header
    auto* pData = audioHeaders_.data();

    if (audioHeaders_.size() < 3) {
        X_ERROR("Video", "Vorbis header too small");
        return false;
    }

    int32_t numPackets = pData[0] + 1;
    if (numPackets != 3) {
        X_ERROR("Video", "Unsupported packed count: %" PRIi32, numPackets);
        return false;
    }

    int32_t idHdrSize = pData[1];
    int32_t commentHdrSize = pData[2];

    auto* pIdHeader = &pData[3];
    auto* pCommentHeader = pIdHeader + idHdrSize;

    X_UNUSED(pIdHeader, pCommentHeader);

    // check block is valid.
    const size_t setupHeaderSize = audioHeaders_.size() - idHdrSize - commentHdrSize;
    if ((idHdrSize + commentHdrSize + setupHeaderSize) != audioHeaders_.size()) {
        X_ERROR("Video", "Invalid vorbis header size");
        return false;
    }

    // compress vorbis header?
    DataVec compData(arena_);

    if (!core::Compression::LZ4HC::deflate(arena_, audioHeaders_, compData, core::Compression::CompressLevel::HIGH)) {
        return false;
    }

    audioTrack_.inflatedHdrSize = safe_static_cast<int32_t>(audioHeaders_.size());
    audioTrack_.deflatedHdrSize = safe_static_cast<int32_t>(compData.size());

    audioHeaders_.swap(compData);

    return true;
}

bool VideoCompiler::writeToFile(core::XFile* pFile) const
{
    if (clusters_.isEmpty()) {
        X_ERROR("Video", "Video has no clusters");
        return false;
    }

    if (durationNS_ <= 0) {
        X_ERROR("Video", "duration is zero");
        return false;
    }

    const int64_t nanosecondsPerSecond = 1000000000;
    const int32_t nanosecondsPerMillisecond = 1000000;

    VideoHeader hdr;
    core::zero_object(hdr);
    hdr.fourCC = VIDEO_FOURCC;
    hdr.version = VIDEO_VERSION;
    hdr.durationNS = durationNS_;
    hdr.video = videoTrack_;
    hdr.audio = audioTrack_;

    if (pFile->writeObj(hdr) != sizeof(hdr)) {
        X_ERROR("Video", "Failed to write data");
        return false;
    }

    // audio headers.
    if (pFile->write(audioHeaders_.data(), audioHeaders_.size()) != audioHeaders_.size()) {
        X_ERROR("Video", "Failed to write data");
        return false;
    }

    // write all the clusters.
    for (const auto& cluster : clusters_)
    {
        ClusterHdr clusterHdr;
        clusterHdr.timeMS = safe_static_cast<int32_t>(cluster.timeNS / nanosecondsPerMillisecond);
        clusterHdr.durationMS = safe_static_cast<int32_t>(cluster.durationNS / nanosecondsPerMillisecond);
        clusterHdr.numBlocks = safe_static_cast<decltype(clusterHdr.numBlocks)>(cluster.blocks.size());

        if (pFile->writeObj(clusterHdr) != sizeof(clusterHdr)) {
            X_ERROR("Video", "Failed to write data");
            return false;
        }

        for (const auto& block : cluster.blocks)
        {
            BlockHdr blockHdr;
            blockHdr.type = block.type;
            blockHdr.isKey = block.isKey;
            blockHdr.timeMS = safe_static_cast<int32_t>(block.timeNS / nanosecondsPerMillisecond);
            blockHdr.blockSize = safe_static_cast<decltype(blockHdr.blockSize)>(block.data.size());

            if (pFile->writeObj(blockHdr) != sizeof(blockHdr)) {
                X_ERROR("Video", "Failed to write data");
                return false;
            }

            if (pFile->write(block.data.data(), block.data.size()) != block.data.size()) {
                X_ERROR("Video", "Failed to write data");
                return false;
            }
        }
    }

    return true;
}

X_NAMESPACE_END