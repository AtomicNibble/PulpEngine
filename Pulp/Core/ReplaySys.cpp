#include "stdafx.h"
#include "ReplaySys.h"

#include <IFrameData.h>
#include <IFileSys.h>

#include <Compression\LZ4.h>

X_NAMESPACE_BEGIN(core)

ReplaySys::ReplaySys(core::MemoryArenaBase* arena) :
    arena_(arena),
    stream_(nullptr, nullptr, false),
    streamData_(arena),
    compData_(arena),
    pFile_(nullptr),
    mode_(Mode::NONE)
{

}

void ReplaySys::setMode(Mode::Enum mode)
{
    if (mode_ == mode) {
        return;
    }
    mode_ = mode;

    streamData_.resize(BUFFER_SIZE + MAX_FRAME_SIZE);

    auto deflateBufSize = core::Compression::LZ4::requiredDeflateDestBuf(streamData_.capacity());
    compData_.resize(deflateBufSize + sizeof(BufferHdr));

    if (mode == Mode::RECORD) {

        stream_ = core::FixedByteStreamNoneOwningPolicy(streamData_.begin(), streamData_.end(), false);

        core::FileFlags flags = core::FileFlag::RECREATE | core::FileFlag::WRITE;
        
        pFile_ = gEnv->pFileSys->openFile(core::Path<>("replay.dat"), flags);
    }
    else if (mode == Mode::PLAY) {

        core::FileFlags flags = core::FileFlag::SHARE | core::FileFlag::READ;

        pFile_ = gEnv->pFileSys->openFile(core::Path<>("replay.dat"), flags);
    }

    core::zero_object(nextEntry_);
    startTime_ = gEnv->pTimer->GetTimeNowNoScale();
}

void ReplaySys::update(FrameInput& inputFrame)
{
    // we either doing nothing, record or replay.
    if (mode_ == Mode::NONE) {
        return;
    }

    static_assert(input::MAX_INPUT_EVENTS_PER_FRAME <= std::numeric_limits<uint8_t>::max(), "Can't store max number of events");

    // we probs want to compress the stream so probs keep a local buffer.
    // then everyso often compress it and write the data.
    // we should not write anything if curpos has not changed.

    if (mode_ == Mode::RECORD) {
         
        EntryHdr hdr;

        if (inputFrame.cusorPos != lastCusorPos_ || inputFrame.cusorPosClient != lastCusorPosClient_) {
            lastCusorPos_ = inputFrame.cusorPos;
            lastCusorPosClient_ = inputFrame.cusorPosClient;

            hdr.flags.Set(DataFlag::CURSOR);
        }

        if (inputFrame.events.isNotEmpty()) {
            hdr.flags.Set(DataFlag::EVENTS);
        }

        if (hdr.flags.IsAnySet()) {

            auto now = gEnv->pTimer->GetTimeNowNoScale();
            auto delta = now - startTime_;

            hdr.msOffset = safe_static_cast<int32_t>(delta.GetMilliSecondsAsInt64());
            stream_.write(hdr);

            if (hdr.flags.IsSet(DataFlag::CURSOR)) {
                stream_.write(inputFrame.cusorPos);
                stream_.write(inputFrame.cusorPosClient);
            }

            if (hdr.flags.IsSet(DataFlag::EVENTS)) {
                stream_.write(safe_static_cast<uint8_t>(inputFrame.events.size()));
                stream_.write(inputFrame.events.data(), inputFrame.events.size());
            }
        }

        if (stream_.size() >= BUFFER_SIZE) {

            compData_.resize(compData_.capacity());

            size_t compDataSize = 0;
            if (!core::Compression::LZ4::deflate(stream_.data(), stream_.size(), compData_.data() + sizeof(BufferHdr), compData_.size() - sizeof(BufferHdr),
                compDataSize, core::Compression::CompressLevel::NORMAL)) {
                X_ERROR("ReplaySys", "Failed to compress replay stream buffer");
                stream_.reset();
                return;
            }

            compData_.resize(compDataSize + sizeof(BufferHdr));

            BufferHdr* pHeader = reinterpret_cast<BufferHdr*>(compData_.data());
            pHeader->inflatedSize = safe_static_cast<uint32_t>(stream_.size());
            pHeader->deflatedSize = safe_static_cast<uint32_t>(compDataSize);
            stream_.reset();

            pFile_->write(compData_.data(), compData_.size());
        }
    }
    else if (mode_ == Mode::PLAY) {

        if (stream_.isEos()) {

            // load another buffer
            // we need to read the header :(
            auto bytesLeft = pFile_->remainingBytes();
            if (bytesLeft == 0) {
                mode_ = Mode::NONE;
                X_LOG0("ReplaySys", "Replay has ended");
                return;
            }

            BufferHdr hdr;
            if (pFile_->readObj(hdr) != sizeof(hdr)) {
                X_ERROR("ReplaySys", "Failed to read entry header");
                return;
            }

            compData_.resize(hdr.deflatedSize);
            streamData_.resize(hdr.inflatedSize);

            pFile_->read(compData_.data(), compData_.size());

            if (!core::Compression::LZ4::inflate(compData_.data(), compData_.size(), 
                streamData_.data(), streamData_.size())) {
                X_ERROR("ReplaySys", "Failed to decompress replay data");
            }

            stream_ = core::FixedByteStreamNoneOwningPolicy(streamData_.begin(), streamData_.end(), true);
            stream_.read(nextEntry_);
        }

        // end of goat?
        X_ASSERT(!stream_.isEos(), "Stream is empty")();

        inputFrame.cusorPos = lastCusorPos_;
        inputFrame.cusorPosClient = lastCusorPosClient_;
        inputFrame.events.clear();

        // read events!
        // we have a stream, but don't know when to play them.
        // i want to peek really.
        auto now = gEnv->pTimer->GetTimeNowNoScale();
        auto delta = now - startTime_;
        auto msOffset = safe_static_cast<int32_t>(delta.GetMilliSecondsAsInt64());

        if(nextEntry_.msOffset <= msOffset) 
        {
            if (nextEntry_.flags.IsSet(DataFlag::CURSOR)) {
                stream_.read(inputFrame.cusorPos);
                stream_.read(inputFrame.cusorPosClient);

                lastCusorPos_ = inputFrame.cusorPos;
                lastCusorPosClient_ = inputFrame.cusorPosClient;
            } 

            if (nextEntry_.flags.IsSet(DataFlag::EVENTS)) {
                auto numEvents = stream_.read<uint8_t>();
                inputFrame.events.resize(numEvents);
                stream_.read(inputFrame.events.data(), inputFrame.events.size());
            }

            if (!stream_.isEos()) {
                stream_.read(nextEntry_);
            }
        }

    }
}


X_NAMESPACE_END