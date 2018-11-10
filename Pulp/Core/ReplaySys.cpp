#include "stdafx.h"
#include "ReplaySys.h"

#include <IFrameData.h>
#include <IFileSys.h>

#include <Compression\LZ4.h>

X_NAMESPACE_BEGIN(core)

namespace
{
    constexpr uint64_t getTrailingBytes(uint64_t offset, uint64_t pageSize)
    {
        return core::bitUtil::RoundUpToMultiple<uint64_t>(offset, pageSize) - offset;
    }

    static_assert(getTrailingBytes(1024, 4096) == 4096 - 1024, "Incorrect size");
    static_assert(getTrailingBytes(4097, 4096) == 4095, "Incorrect size");
    static_assert(getTrailingBytes(4096, 4096) == 0, "Incorrect size");


} // namespace

ReplaySys::ReplaySys(core::MemoryArenaBase* arena) :
    arena_(arena),
    stream_(nullptr, nullptr, false),
    streamData_(arena),
    compData_(arena),
    signal_(true),
    pFile_(nullptr),
    fileSize_(0),
    readOffset_(0),
    mode_(Mode::NONE)
{
}

ReplaySys::~ReplaySys()
{
    stop();

    X_ASSERT(pFile_ == nullptr, "File was not closed")(pFile_);
}

void ReplaySys::record(const core::string& name)
{
    stop();

    core::Path<> path;
    path.append(name.begin(), name.end());
    path.setExtension(".rec");

    pFile_ = gEnv->pFileSys->openFileAsync(path, core::FileFlag::RECREATE | core::FileFlag::WRITE);
    if (!pFile_) {
        X_ERROR("ReplaySys", "Failed to open replay output file");
        return;
    }

    // for rec it's resize not reserver
    streamData_.resize(MAX_STREAM_SIZE);
    stream_ = core::FixedByteStreamNoneOwningPolicy(streamData_.begin(), streamData_.end(), false);
    compData_.reserve(core::Compression::LZ4::requiredDeflateDestBuf(streamData_.capacity()) + sizeof(BufferHdr));

    mode_ = Mode::RECORD;
    startTime_ = gEnv->pTimer->GetTimeNowNoScale();
    X_LOG0("ReplaySys", "Started recording replay \"%s\"", name.c_str());
}

void ReplaySys::play(const core::string& name)
{
    stop();

    core::Path<> path;
    path.append(name.begin(), name.end());
    path.setExtension(".rec");

    pFile_ = gEnv->pFileSys->openFileAsync(path, core::FileFlag::READ | core::FileFlag::SHARE);
    if (!pFile_) {
        X_ERROR("ReplaySys", "Failed to open replay");
        return;
    }

    fileSize_ = pFile_->fileSize();
    if (fileSize_ == 0) {
        stop();
        X_ERROR("ReplaySys", "Replay file is empty");
        return;
    }

    X_ASSERT(stream_.isEos(), "Stream has not been reset")();
    core::zero_object(nextEntry_);
    
    streamData_.reserve(MAX_STREAM_SIZE);
    compData_.reserve(core::Compression::LZ4::requiredDeflateDestBuf(streamData_.capacity()) + sizeof(BufferHdr));

    mode_ = Mode::PLAY;
    startTime_ = gEnv->pTimer->GetTimeNowNoScale();
    X_LOG0("ReplaySys", "Started playing replay \"%s\"", name.c_str());
}

void ReplaySys::stop(void)
{
    if (mode_ == Mode::NONE) {
        return;
    }

    if (mode_ == Mode::RECORD) {
        if (stream_.size() > 0) {
            // Flush it baby!
            dispatchWrite();
            signal_.wait();
        }
        else {
            // wait for any pending IO
            if (pendingIO_) {
                signal_.wait();
            }
        }
    }

    X_ASSERT(pendingIO_ == 0, "Pending IO")();

    streamData_.clear();
    compData_.clear();
    stream_.reset();
    startTime_ = core::TimeVal();

    if (pFile_) {
        gEnv->pFileSys->closeFileAsync(pFile_);
        pFile_ = nullptr;
    }

    signal_.clear();
    fileSize_ = 0;
    readOffset_ = 0;

    mode_ = Mode::NONE;
}

void ReplaySys::update(FrameInput& inputFrame)
{
    if (mode_ == Mode::NONE) {
        return;
    }

    static_assert(input::MAX_INPUT_EVENTS_PER_FRAME <= std::numeric_limits<uint8_t>::max(), "Can't store max number of events");

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
            hdr.msOffset = getOffsetMS();
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
            dispatchWrite();
        }
    }
    else if (mode_ == Mode::PLAY) {

        if (stream_.isEos()) {
            X_ASSERT(readOffset_ <= fileSize_, "Offset greater than size")(readOffset_, fileSize_);

            auto bytesLeft = fileSize_ - readOffset_;
            if (bytesLeft == 0) {
                X_LOG0("ReplaySys", "Replay has ended");
                stop();
                return;
            }

            // read the header, but read in a few pages after see if we get lucky add get all the data we need.
            auto pageSize = 4096;
            auto pageTrailingBytes = getTrailingBytes(readOffset_ + sizeof(BufferHdr), pageSize);
            auto readSize = core::Min(sizeof(BufferHdr) + pageTrailingBytes + pageSize * 2, bytesLeft);

            X_ASSERT(readSize >= sizeof(BufferHdr), "File is not empty, but no space for header")(readSize, bytesLeft);

            compData_.resize(readSize);

            core::IoRequestRead r;
            r.callback.Bind<ReplaySys, &ReplaySys::IoRequestCallback>(this);
            r.pFile = pFile_;
            r.pBuf = compData_.data();
            r.offset = readOffset_;
            r.dataSize = safe_static_cast<uint32_t>(compData_.size());
            gEnv->pFileSys->AddIoRequestToQue(r);
            pendingIO_ = 1;

            // wait :( ?
            signal_.wait();

            // make copy
            auto hdr = *reinterpret_cast<const BufferHdr*>(compData_.data());
            X_ASSERT(hdr.inflatedSize <= MAX_STREAM_SIZE, "Inflated size is bigger than max")();

            auto compBytesRead = compData_.size() - sizeof(BufferHdr);
            if (hdr.deflatedSize < compBytesRead)
            {
                // we have all the data, trim buffer.
                auto numBytes = hdr.deflatedSize + sizeof(BufferHdr);
                readOffset_ += numBytes;
                compData_.resize(numBytes);

                X_ASSERT(readOffset_ <= fileSize_, "Offset greater than size")(readOffset_, fileSize_);
            }
            else
            {
                // read the rest
                auto bytesRead = compData_.size();
                auto remaningCompBytes = hdr.deflatedSize - compBytesRead;
                auto bufferSize = hdr.deflatedSize + sizeof(BufferHdr);

                // we are appending data, we must make sure the data is not thrown away.
                X_ASSERT(compData_.capacity() >= bufferSize, "Capacity is smaller than requried size, realloc will occur")(compData_.capacity(), bufferSize);
                compData_.resize(bufferSize);

                r.pBuf = compData_.data() + bytesRead;
                r.offset = readOffset_ + bytesRead;
                r.dataSize = safe_static_cast<uint32_t>(remaningCompBytes);
                gEnv->pFileSys->AddIoRequestToQue(r);
                pendingIO_ = 1;

                readOffset_ += bufferSize;
                X_ASSERT(readOffset_ <= fileSize_, "Offset greater than size")(readOffset_, fileSize_);

                signal_.wait();
            }

            streamData_.resize(hdr.inflatedSize);

            if (!core::Compression::LZ4::inflate(compData_.data() + sizeof(BufferHdr), compData_.size() - sizeof(BufferHdr),
                streamData_.data(), streamData_.size())) {
                X_ERROR("ReplaySys", "Failed to decompress replay data");
                stop();
                return;
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
        auto msOffset = getOffsetMS();

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

void ReplaySys::dispatchWrite(void)
{
    if (stream_.size() == 0) {
        return;
    }

    // make sure the last IO request has finished.
    // typically this should never wait unless you have very high frame rate like 100+
    if (compData_.isNotEmpty()) {
        signal_.wait();
    }

    X_ASSERT(pendingIO_ == 0, "IO is pending")();

    compData_.resize(compData_.capacity());

    // could make this a job, but should not be slow.
    size_t compDataSize = 0;
    if (!core::Compression::LZ4::deflate(stream_.data(), stream_.size(), compData_.data() + sizeof(BufferHdr), compData_.size() - sizeof(BufferHdr),
        compDataSize, core::Compression::CompressLevel::NORMAL)) {
        X_ERROR("ReplaySys", "Failed to compress replay stream buffer");
        stop();
        return;
    }

    compData_.resize(compDataSize + sizeof(BufferHdr));

    BufferHdr* pHeader = reinterpret_cast<BufferHdr*>(compData_.data());
    pHeader->inflatedSize = safe_static_cast<uint32_t>(stream_.size());
    pHeader->deflatedSize = safe_static_cast<uint32_t>(compDataSize);
    stream_.reset();

    core::IoRequestWrite r;
    r.callback.Bind<ReplaySys, &ReplaySys::IoRequestCallback>(this);
    r.pFile = pFile_;
    r.pBuf = compData_.data();
    r.offset = fileSize_;
    r.dataSize = safe_static_cast<uint32_t>(compData_.size());

    gEnv->pFileSys->AddIoRequestToQue(r);
    pendingIO_ = 1;

    fileSize_ += compData_.size();
}

int32_t ReplaySys::getOffsetMS(void) const
{
    auto now = gEnv->pTimer->GetTimeNowNoScale();
    auto delta = now - startTime_;
    return safe_static_cast<int32_t>(delta.GetMilliSecondsAsInt64());
}

void ReplaySys::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
    core::XFileAsync* pFile, uint32_t bytesTransferred)
{
    X_UNUSED(fileSys, pFile);

    auto type = pRequest->getType();
    X_ASSERT(type == core::IoRequest::READ || type == core::IoRequest::WRITE, "Invalid request type")(type);

    if (type == core::IoRequest::READ) {
        const auto* pReq = static_cast<const core::IoRequestRead*>(pRequest);
        if (pReq->dataSize != bytesTransferred) {
            X_ERROR("ReplaySys", "Failed to read replay data");
        }

    } else if (type == core::IoRequest::WRITE) {
        const auto* pReq = static_cast<const core::IoRequestWrite*>(pRequest);
        if (pReq->dataSize != bytesTransferred) {
            X_ERROR("ReplaySys", "Failed to write replay data");
        }

    }
    else {
        X_ASSERT_UNREACHABLE();
    }
    
    pendingIO_ = 0;
    signal_.raise();
}

X_NAMESPACE_END