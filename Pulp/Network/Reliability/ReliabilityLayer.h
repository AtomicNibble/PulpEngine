#pragma once

#include "Util\BPSTracker.h"
#include "Util\RangeList.h"

#include <Containers\Array.h>
#include <Containers\Fifo.h>
#include <Containers\LinkedListIntrusive.h>
#include <Containers\PriorityQueue.h>

#include <Util\ReferenceCounted.h>
#include <Util\UniquePointer.h>

#include <functional>

X_NAMESPACE_BEGIN(net)

class NetSocket;
class NetVars;

typedef uint16_t SplitPacketId;
typedef uint32_t SplitPacketIndex;
typedef uint16_t MessageNumber;
typedef uint16_t OrderingIndex;
typedef uint16_t DataGramSequenceNumber;

struct ReliablePacket;

// a channel for a split pack to reassembler itself into.
struct SplitPacketChannel
{
    typedef core::Array<ReliablePacket*> PacketArr;

public:
    SplitPacketChannel(core::MemoryArenaBase* arena);

    bool hasFirstPacket(void) const;
    bool haveAllPackets(void) const;

public:
    SplitPacketId splitId;
    SplitPacketIndex packetsRecived;
    core::TimeVal lastUpdate;
    PacketArr packets;
};

struct DataRefrence : public core::ReferenceCounted<>
{
    uint8_t* pData;
};

struct ReliablePacket
{
    X_DECLARE_ENUM(DataType)
    (
        Normal,
        Ref // the data is refrenced, this is so we can split large blocks into multiple packets without copyies.
    );

    X_NO_COPY(ReliablePacket);
    X_NO_ASSIGN(ReliablePacket);

public:
    ReliablePacket(core::MemoryArenaBase* dataArena);
    ~ReliablePacket();

    void freeData(void);
    void allocData(size_t numBits);
    core::MemoryArenaBase* getArena(void) const;
    void setArena(core::MemoryArenaBase* arena);

    bool isReliable(void) const;
    bool isAckRequired(void) const;
    bool isOrderedOrSequenced(void) const;
    bool isSequenced(void) const;
    bool hasSplitPacket(void) const;
    size_t getHeaderLengthBits(void) const; // returns number of bits needed to store this header. it's diffrent depending on priority types etc.

    void writeToBitStream(core::FixedBitStreamBase& bs) const;
    bool fromBitStream(core::FixedBitStreamBase& bs);

    // cops values of fields excluding data ones.
    void assignPropertiesExcData(const ReliablePacket* pOth);

private:
    PacketReliability::Enum reliabilityWithoutAck(void) const;

public:
    static constexpr size_t getMaxHeaderLength(void);
    static constexpr size_t getMaxHeaderLengthBits(void);

public:
    MessageNumber reliableMessageNumber;
    OrderingIndex orderingIndex;
    OrderingIndex sequencingIndex;

    PacketReliability::Enum reliability;
    PacketPriority::Enum priority;
    uint8_t sendAttemps;
    SendReceipt sendReceipt;

    uint8_t orderingChannel;
    SplitPacketId splitPacketId;
    SplitPacketIndex splitPacketIndex;
    SplitPacketIndex splitPacketCount; // uses index type, so index type can represent any count value.

    core::TimeVal creationTime;       //
    core::TimeVal retransmissionTime; //
    core::TimeVal nextActionTime;     //

    DataType::Enum dataType;
    BitSizeT dataBitLength;
    uint8_t* pData;
    DataRefrence* pRefData;

    INTRUSIVE_LIST_LINK(ReliablePacket)
    reliableLink;

private:
    core::MemoryArenaBase* arena; // arena this memory is from.
};

static const size_t goat = sizeof(ReliablePacket);
// static_assert(core::compileTime::IsPOD<RelPacket>::Value, "Packet header should be POD");

struct DelayedPacket
{
    DelayedPacket(NetSocket* pSocket, core::TimeVal sendTime, size_t sizeInBytes, uint8_t* pData, core::MemoryArenaBase* arena) :
        pSocket(pSocket),
        sendTime(sendTime),
        sizeInBytes(sizeInBytes),
        data(arena, pData)
    {
    }

    X_INLINE bool operator<(const DelayedPacket& oth) const
    {
        return sendTime < oth.sendTime;
    }
    X_INLINE bool operator>(const DelayedPacket& oth) const
    {
        return sendTime > oth.sendTime;
    }

    NetSocket* pSocket;
    core::TimeVal sendTime; // time to send this.
    size_t sizeInBytes;
    core::UniquePointer<uint8_t[]> data;
};

struct DataGramHistory
{
    typedef core::FixedArray<MessageNumber, MAX_REL_PACKETS_PER_DATAGRAM> MesgNumberArr;

public:
    DataGramHistory(core::TimeVal timesent) :
        timeSent(timesent)
    {
#if X_DEBUG
        magic = 0x12345678;
#endif // X_DEBUG
    }

#if X_DEBUG
    uint32_t magic;
#endif // X_DEBUG

    core::TimeVal timeSent;
    MesgNumberArr messagenumbers;
};

struct PacketData
{
    X_INLINE PacketData(core::MemoryArenaBase* arena);
    X_INLINE ~PacketData();

    X_INLINE void setdata(uint8_t* pData, BitSizeT numBits, core::MemoryArenaBase* arena);
    X_INLINE core::UniquePointer<uint8_t[]>& getUP(void);

    X_INLINE BitSizeT getNumbBits(void) const;
    X_INLINE uint8_t* getData(void) const;

    X_INLINE uint8_t* begin(void);
    X_INLINE uint8_t* end(void);

    X_INLINE const uint8_t* begin(void) const;
    X_INLINE const uint8_t* end(void) const;

private:
    BitSizeT numBits_;
    core::UniquePointer<uint8_t[]> data_;
};

class ReliabilityLayer
{
    typedef std::array<OrderingIndex, MAX_ORDERED_STREAMS> OrdereIndexArr;
    typedef std::array<ReliablePacket*, REL_RESEND_BUF_LENGTH> ResendArr;
    typedef core::Fifo<DataGramHistory> DataGramHistoryQeue;
    typedef core::Fifo<bool> BoolQeue;
    typedef core::Fifo<ReliablePacket*> PacketQeue;
    typedef core::Array<SplitPacketChannel*> SplitPacketChannelArr;
    typedef core::Array<ReliablePacket*> RelPacketArr;
    typedef core::Array<int32_t> FrameboundryArr;
    typedef core::PriorityQueue<DelayedPacket, core::Array<DelayedPacket>, std::greater<>> OrderedDelaysPacketQueue;

    typedef uint64_t WeightType;

    struct PacketWithWeight
    {
        X_INLINE PacketWithWeight(WeightType wht, ReliablePacket* pPacket) :
            weight(wht),
            pPacket(pPacket)
        {
        }

        X_INLINE bool operator<(const PacketWithWeight& rhs) const
        {
            return weight < rhs.weight;
        }
        X_INLINE bool operator>(const PacketWithWeight& rhs) const
        {
            return weight > rhs.weight;
        }

        WeightType weight;
        ReliablePacket* pPacket;
    };

    typedef core::PriorityQueue<PacketWithWeight, core::Array<PacketWithWeight>, std::greater<PacketWithWeight>> OrderedPacketQueue;
    typedef std::array<OrderedPacketQueue, MAX_ORDERED_STREAMS> OrderedPacketQueues;

    X_DECLARE_ENUM(ProcessResult)
    (
        Ok,
        Ignored);

public:
    typedef RangeList<DataGramSequenceNumber> DataGramNumberRangeList;
    typedef PacketData PacketData;

    X_NO_COPY(ReliabilityLayer);
    X_NO_ASSIGN(ReliabilityLayer);

public:
    ReliabilityLayer(NetVars& vars, core::MemoryArenaBase* genArena, core::MemoryArenaBase* packetDataArena, core::MemoryArenaBase* packetPool);
    ReliabilityLayer(ReliabilityLayer&& oth) = default;
    ~ReliabilityLayer();

    ReliabilityLayer& operator=(ReliabilityLayer&& rhs) = default;

    void free(void);
    void reset(int32_t MTUSize);

    // que some data for sending, reliability is handled.
    bool send(const uint8_t* pData, const BitSizeT lengthBits, core::TimeVal time, uint32_t mtuSize,
        PacketPriority::Enum priority, PacketReliability::Enum reliability, uint8_t orderingChannel, SendReceipt receipt, bool ownData);

    // pass data from socket for processing
    bool recv(uint8_t* pData, const size_t lengt, NetSocket& socket,
        SystemAddressEx& systemAddress, core::TimeVal time, uint32_t mtuSize);

    // update internal logic, re-send packets / other reliability actions.
    void update(core::FixedBitStreamBase& bs, NetSocket& socket, SystemAddressEx& systemAddress, int32_t MTUSize,
        core::TimeVal time);

    // pop any packets that have arrived.
    bool recive(PacketData& dataOut);

    void getStatistics(NetStatistics& stats);
    void getStatistics(NetStatistics& stats) const;

    X_INLINE bool pendingOutgoingData(void) const;
    X_INLINE bool isWaitingForAcks(void) const;
    X_INLINE bool isConnectionDead(void) const;
    X_INLINE void killConnection(void);

private:
    void clearPacketQueues(void);
    size_t calculateMemoryUsage(void) const;

private:
    ProcessResult::Enum prcoessIncomingPacket(ReliablePacket*& pPacketInOut, core::TimeVal time);
    void ignorePacket(ReliablePacket* pPacket, core::TimeVal time);
    void addPacketToRecivedQueue(ReliablePacket* pPacket, core::TimeVal time);

private:
    X_INLINE void addAck(DataGramSequenceNumber messageNumber);
    size_t maxDataGramSize(void) const;
    size_t maxDataGramSizeExcHdr(void) const;
    size_t dataGramHdrSize(void) const;
    size_t dataGramHdrSizeBits(void) const;

    X_INLINE BitSizeT maxDataGramSizeBits(void) const;
    X_INLINE BitSizeT maxDataGramSizeExcHdrBits(void) const;

private:
    bool hasTimedOut(core::TimeVal time);

    void sendACKs(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAddressEx& systemAddress, core::TimeVal time);
    void sendNAKs(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAddressEx& systemAddress, core::TimeVal time);
    void sendBitStream(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAddressEx& systemAddress, core::TimeVal time);

    ReliablePacket* packetFromBS(core::FixedBitStreamBase& bs, core::TimeVal time);
    ReliablePacket* allocPacket(void);
    SplitPacketChannel* allocSplicPacketChannel(void);
    void freePacket(ReliablePacket* pPacket);
    bool splitPacket(ReliablePacket* pPacket);
    void freeSplitPacketChannel(SplitPacketChannel* pSPC);

    // recived split packets.
    ReliablePacket* addIncomingSplitPacket(ReliablePacket* pPacket, core::TimeVal time);

private:
    // these are created each time a data gram is sent.
    // so if a packet gets resent a new one of these will be made.
    DataGramHistory* createDataGramHistory(DataGramSequenceNumber number, core::TimeVal time);
    DataGramHistory* getDataGramHistory(DataGramSequenceNumber number);
    bool clearDataGramHistory(DataGramSequenceNumber number);

private:
    X_INLINE size_t resendBufferIdxForMsgNum(MessageNumber msgNum) const;

    X_INLINE bool isResendBufferFull(void) const; // checks if current index is occupied.
    X_INLINE bool isResendListEmpty(void) const;
    void setResendBuffer(size_t idx, ReliablePacket* pPacket);
    void clearResendBuffer(size_t idx);
    void insertPacketToResendList(ReliablePacket* pPacket);
    void movePacketToTailOfResendList(ReliablePacket* pPacket);
    void removePacketFromResendList(MessageNumber msgNum);
    void freeResendList(void);

    INTRUSIVE_LIST_DECLARE(ReliablePacket, reliableLink) resendList_;

    X_INLINE bool isOlderPacket(OrderingIndex packetIdx, OrderingIndex currentIdx);

private:
    NetVars& vars_;

    core::MemoryArenaBase* arena_;
    core::MemoryArenaBase* packetDataArena_;
    core::MemoryArenaBase* packetPool_;

    int32_t MTUSize_;

    core::TimeVal timeLastDatagramArrived_;
    core::TimeVal lastBSPUpdate_;

    OrdereIndexArr orderedWriteIndex_;         // inc for every ordered msg sent
    OrdereIndexArr sequencedWriteIndex_;       // inc for every sequenced msg sent, reset if orderedWriteIndex_ changes.
    OrdereIndexArr orderedReadIndex_;          // next 'expected' ordered index
    OrdereIndexArr highestSequencedReadIndex_; // higest value received for sequencedWriteIndex, for current orderedReadIndex_.
    OrdereIndexArr orderingQueueIndexOffset_;  // used to make relative weights.
    OrderedPacketQueues orderingQueues_;       // used for storing out of order packets.

    PacketQeue outGoingPackets_;
    PacketQeue recivedPackets_;
    DataGramHistoryQeue dataGramHistory_;
    DataGramSequenceNumber dataGramHistoryPopCnt_;
    DataGramSequenceNumber recivedPacketBaseIdx_; // the number we are expecting and
    BoolQeue recivedPacketQueue_;

    DataGramNumberRangeList incomingAcks_;
    DataGramNumberRangeList naks_;
    DataGramNumberRangeList acks_;

    MessageNumber reliableMessageNumberIdx_; // current rel msg number index.
    MessageNumber dagramSeqNumber_;
    SplitPacketId splitPacketId_;
    SplitPacketChannelArr splitPacketChannels_;

    // per udate buffers, here to keep bufferes around.
    RelPacketArr packetsThisFrame_;
    FrameboundryArr packetsThisFrameBoundaries_;

    bool connectionDead_;
    bool _pad[3];

    BPSTracker bps_[NetStatistics::Metric::ENUM_COUNT];

    NetStatistics::PriorityMsgCountsArr msgInSendBuffers_;
    NetStatistics::PriorityByteCountsArr bytesInSendBuffers_;
    size_t bytesInReSendBuffers_;
    size_t msgInReSendBuffers_;

    // for artifical ping only.
    OrderedDelaysPacketQueue delayedPackets_;

    // this is quite fat, so left on end
    ResendArr resendBuf_; // max in transit
};

X_NAMESPACE_END

#include "ReliabilityLayer.inl"