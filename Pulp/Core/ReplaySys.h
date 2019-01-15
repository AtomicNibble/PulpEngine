#pragma once

#include <Containers\FixedByteStream.h>
#include <Time\TimeVal.h>
#include <Threading\Signal.h>

X_NAMESPACE_BEGIN(core)

struct XFileAsync;
struct IoRequestBase;

struct FrameInput;

class ReplaySys
{
public:
    X_DECLARE_ENUM(Mode)(
        NONE,
        RECORD,
        PLAY
    );

    X_DECLARE_FLAGS8(DataFlag)(
        CURSOR,
        EVENTS
    );

    typedef Flags8<DataFlag> DataFlags;

    static const size_t MAX_FRAME_SIZE = 256 + (input::MAX_INPUT_EVENTS_PER_FRAME * sizeof(input::InputEvent));
    static const size_t BUFFER_SIZE = (1024 * 128); // higer = more memory but maybe better compression.
    static const size_t MAX_STREAM_SIZE = BUFFER_SIZE + MAX_FRAME_SIZE;

    X_PACK_PUSH(1)

    struct EntryHdr
    {
        int32_t msOffset;
        DataFlags flags;
    };

    struct BufferHdr
    {
        uint32_t inflatedSize;
        uint32_t deflatedSize;
    };

    X_PACK_POP

    X_ENSURE_SIZE(EntryHdr, 5);
    X_ENSURE_SIZE(BufferHdr, 8);

public:
    ReplaySys(core::MemoryArenaBase* arena);
    ~ReplaySys();

    void record(core::string_view name);
    void play(core::string_view name);
    void stop(void);

    void update(FrameInput& inputFrame);

private:
    void dispatchWrite(void);

    int32_t getOffsetMS(void) const;

    void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
        core::XFileAsync* pFile, uint32_t bytesTransferred);

private:
    core::MemoryArenaBase* arena_;
    core::FixedByteStreamNoneOwningPolicy stream_;
    core::Array<uint8_t> streamData_;
    core::Array<uint8_t> compData_;

    core::Signal signal_;
    core::XFileAsync* pFile_;
    uint64_t fileSize_;
    uint64_t readOffset_;
    core::AtomicInt pendingIO_;

    Mode::Enum mode_;

    core::TimeVal startTime_;
    EntryHdr nextEntry_;

    Vec2i lastCusorPos_;
    Vec2i lastCusorPosClient_;

};


X_NAMESPACE_END