#include "stdafx.h"

#include <Memory\AllocationPolicies\StackAllocator.h>
#include <Time\StopWatch.h>
#include <String\HumanSize.h>
#include <Random\MultiplyWithCarry.h>

#include <ITimer.h>

#include "ReliabilityLayer.h"

#include "Sockets\Socket.h"
#include "Vars\NetVars.h"

X_NAMESPACE_BEGIN(net)

namespace
{
    // move to bit util maybe :)

    X_DECLARE_FLAGS(DatagramFlag)
    (
        Ack,
        Nack);

    typedef Flags<DatagramFlag> DatagramFlags;

    X_PACK_PUSH(1)
    struct DatagramHdr
    {
        void writeToBitStream(core::FixedBitStreamBase& bs) const;
        void fromBitStream(core::FixedBitStreamBase& bs);

        uint16_t number;
        DatagramFlags flags;
    };
    X_PACK_POP;

    void DatagramHdr::writeToBitStream(core::FixedBitStreamBase& bs) const
    {
        bs.write(number);
        bs.write(flags);
        bs.alignWriteToByteBoundry();
    }

    void DatagramHdr::fromBitStream(core::FixedBitStreamBase& bs)
    {
        bs.read(number);
        bs.read(flags);
        bs.alignReadToByteBoundry();
    }

} // namespace

// ----------------------------------------------------------

SplitPacketChannel::SplitPacketChannel(core::MemoryArenaBase* arena) :
    splitId(0),
    packetsRecived(0),
    packets(arena)
{
}

bool SplitPacketChannel::hasFirstPacket(void) const
{
    X_ASSERT(packets.isNotEmpty(), "Should not be empty")(packets.size(), packets.capacity());
    return packets[0] != nullptr;
}

bool SplitPacketChannel::haveAllPackets(void) const
{
    X_ASSERT(packets.isNotEmpty(), "Should not be empty")(packets.size(), packets.capacity());
    return hasFirstPacket() && packets[0]->splitPacketCount == packetsRecived;
}

// ----------------------------------------------------------

ReliablePacket::ReliablePacket(core::MemoryArenaBase* dataArena)
{
    reliableMessageNumber = std::numeric_limits<decltype(reliableMessageNumber)>::max();
    orderingIndex = 0;
    sequencingIndex = 0;

    // leave pri / rel unassigned.
    sendAttemps = 0;
    sendReceipt = 0;

    orderingChannel = 0;
    splitPacketId = 0;
    splitPacketIndex = 0;
    splitPacketCount = 0;

    dataType = DataType::Normal;
    dataBitLength = 0;
    pData = nullptr;
    pRefData = nullptr;
    arena = dataArena;
}

ReliablePacket::~ReliablePacket()
{
    X_ASSERT(pData == nullptr, "Data was not free before packet decon")(pData);
}

void ReliablePacket::freeData(void)
{
    // MEOW.
    if (dataType == DataType::Normal) {
        if (pData) {
            X_DELETE_ARRAY(pData, arena);
            pData = nullptr;
            dataBitLength = 0;
        }
        else {
            X_ASSERT(dataBitLength == 0, "No data buffer, when length is none zero")(dataBitLength, pData);
        }
    }
    else if (dataType == DataType::Ref) {
        X_ASSERT_NOT_NULL(pRefData); // only allow ref to be set if it's valid ref

        if (pRefData->removeReference() == 0) {
            // delete original allocation.
            X_DELETE_ARRAY(pRefData->pData, arena);

            X_DELETE(pRefData, g_NetworkArena);
        }

        pData = nullptr;
        pRefData = nullptr;
        dataBitLength = 0;
    }
    else {
        X_ASSERT_UNREACHABLE();
    }
}

void ReliablePacket::allocData(size_t numBits)
{
    // do i just want to set dataType in here? or just assume it's always normal and check with assert :/
    X_ASSERT(dataType == DataType::Normal, "Expected data type to by normal")(DataType::ToString(dataType));
    X_ASSERT(pData == nullptr, "Packet already has data")(pData, dataBitLength);
    X_ASSERT_NOT_NULL(arena);

    dataBitLength = safe_static_cast<BitSizeT>(numBits);
    pData = X_NEW_ARRAY_ALIGNED(uint8_t, core::bitUtil::bitsToBytes(numBits), arena, "PacketData", 16);
}

core::MemoryArenaBase* ReliablePacket::getArena(void) const
{
    return arena;
}

void ReliablePacket::setArena(core::MemoryArenaBase* arena)
{
    X_ASSERT(pData == nullptr, "Packet already has data")(pData, dataBitLength);

    this->arena = arena;
}

bool ReliablePacket::isReliable(void) const
{
    switch (reliability) {
        case PacketReliability::Reliable:
        case PacketReliability::ReliableOrdered:
        case PacketReliability::ReliableOrderedWithAck:
        case PacketReliability::ReliableSequenced:
        case PacketReliability::ReliableWithAck:
            return true;

        default:
            return false;
    }
}

bool ReliablePacket::isAckRequired(void) const
{
    switch (reliability) {
        case PacketReliability::UnReliableWithAck:
        case PacketReliability::ReliableWithAck:
        case PacketReliability::ReliableOrderedWithAck:
            return true;

        default:
            return false;
    }
}

bool ReliablePacket::isOrderedOrSequenced(void) const
{
    switch (reliability) {
        case PacketReliability::UnReliableSequenced:
        case PacketReliability::ReliableSequenced:
        case PacketReliability::ReliableOrdered:
        case PacketReliability::ReliableOrderedWithAck:
            return true;

        default:
            return false;
    }
}

bool ReliablePacket::isSequenced(void) const
{
    switch (reliability) {
        case PacketReliability::UnReliableSequenced:
        case PacketReliability::ReliableSequenced:
            return true;

        default:
            return false;
    }
}

bool ReliablePacket::hasSplitPacket(void) const
{
    // use count, id may be set with a value of 0.
    return splitPacketCount != 0;
}

size_t ReliablePacket::getHeaderLengthBits(void) const
{
    size_t bits = 0;

    bits += core::bitUtil::bitsNeededForValue(PacketReliability::ENUM_COUNT);
    bits += 1;                                           // bool for splitPacket.
    bits = core::bitUtil::RoundUpToMultiple(bits, 8_sz); // we align write.
    bits += core::bitUtil::bytesToBits(sizeof(uint16_t));

    if (reliability == PacketReliability::Reliable || reliability == PacketReliability::ReliableOrdered || reliability == PacketReliability::ReliableSequenced || reliability == PacketReliability::ReliableOrderedWithAck || reliability == PacketReliability::ReliableWithAck) {
        bits += core::bitUtil::bytesToBits(sizeof(decltype(reliableMessageNumber)));
    }

    // sequenced.
    if (reliability == PacketReliability::UnReliableSequenced || reliability == PacketReliability::ReliableSequenced) {
        bits += core::bitUtil::bytesToBits(sizeof(decltype(sequencingIndex)));
    }

    // ordered
    if (reliability == PacketReliability::UnReliableSequenced || reliability == PacketReliability::ReliableSequenced || reliability == PacketReliability::ReliableOrdered || reliability == PacketReliability::ReliableOrderedWithAck) {
        bits += core::bitUtil::bytesToBits(sizeof(decltype(orderingIndex)));
        bits += core::bitUtil::bytesToBits(sizeof(decltype(orderingChannel)));
    }

    if (hasSplitPacket()) {
        bits += core::bitUtil::bytesToBits(sizeof(decltype(splitPacketId)));
        bits += core::bitUtil::bytesToBits(sizeof(decltype(splitPacketIndex)));
        bits += core::bitUtil::bytesToBits(sizeof(decltype(splitPacketCount)));
    }

    bits = core::bitUtil::RoundUpToMultiple(bits, 8_sz);

    X_ASSERT(bits <= getMaxHeaderLengthBits(), "bit count exceeded calculated max")(bits, getMaxHeaderLengthBits());
    return bits;
}

constexpr size_t ReliablePacket::getMaxHeaderLength(void)
{
    // calculated at compile time.
    return core::bitUtil::bitsToBytes(getMaxHeaderLengthBits());
}

constexpr size_t ReliablePacket::getMaxHeaderLengthBits(void)
{
    // calculated at compile time.
    return core::bitUtil::RoundUpToMultiple<size_t>(
        core::bitUtil::RoundUpToMultiple<size_t>(
            core::bitUtil::bitsNeededForValue(PacketReliability::ENUM_COUNT) + 1,
            8)
            + core::bitUtil::bytesToBits(sizeof(uint16_t)) + core::bitUtil::bytesToBits(sizeof(decltype(reliableMessageNumber))) + core::bitUtil::bytesToBits(sizeof(decltype(sequencingIndex))) + core::bitUtil::bytesToBits(sizeof(decltype(orderingIndex))) + core::bitUtil::bytesToBits(sizeof(decltype(orderingChannel))) + core::bitUtil::bytesToBits(sizeof(decltype(splitPacketId))) + core::bitUtil::bytesToBits(sizeof(decltype(splitPacketIndex))) + core::bitUtil::bytesToBits(sizeof(decltype(splitPacketCount))),
        8);
}

void ReliablePacket::writeToBitStream(core::FixedBitStreamBase& bs) const
{
    auto rel = reliabilityWithoutAck();

    bs.writeBits(reinterpret_cast<const uint8_t*>(&rel), core::bitUtil::bitsNeededForValue(PacketReliability::ENUM_COUNT));
    bs.write<bool>(hasSplitPacket());
    bs.writeAligned<uint16_t>(safe_static_cast<uint16_t>(dataBitLength));

    // reliable types.
    if (reliability == PacketReliability::Reliable || reliability == PacketReliability::ReliableOrdered || reliability == PacketReliability::ReliableSequenced || reliability == PacketReliability::ReliableOrderedWithAck || reliability == PacketReliability::ReliableWithAck) {
        bs.write(reliableMessageNumber);
    }

    // sequenced.
    if (reliability == PacketReliability::UnReliableSequenced || reliability == PacketReliability::ReliableSequenced) {
        bs.write(sequencingIndex);
    }

    // ordered
    if (reliability == PacketReliability::UnReliableSequenced || reliability == PacketReliability::ReliableSequenced || reliability == PacketReliability::ReliableOrdered || reliability == PacketReliability::ReliableOrderedWithAck) {
        bs.write(orderingIndex);
        bs.write(orderingChannel);
    }

    if (hasSplitPacket()) {
        bs.write(splitPacketId);
        bs.write(splitPacketIndex);
        bs.write(splitPacketCount);
    }

    bs.writeAligned(pData, core::bitUtil::bitsToBytes(dataBitLength));
}

bool ReliablePacket::fromBitStream(core::FixedBitStreamBase& bs)
{
    X_ASSERT_NOT_NULL(arena);

    uint8_t rel;
    uint16_t bits;
    bs.readBits(&rel, core::bitUtil::bitsNeededForValue(PacketReliability::ENUM_COUNT));
    bool splitPacket = bs.readBool();
    bs.readAligned(bits);

    reliability = static_cast<PacketReliability::Enum>(rel);

    // reliable types.
    if (reliability == PacketReliability::Reliable || reliability == PacketReliability::ReliableOrdered || reliability == PacketReliability::ReliableSequenced || reliability == PacketReliability::ReliableOrderedWithAck || reliability == PacketReliability::ReliableWithAck) {
        bs.read(reliableMessageNumber);
    }
    else {
#if X_DEBUG
        reliableMessageNumber = std::numeric_limits<decltype(reliableMessageNumber)>::max();
#endif // !X_DEBUG
    }

    // sequenced.
    if (reliability == PacketReliability::UnReliableSequenced || reliability == PacketReliability::ReliableSequenced) {
        bs.read(sequencingIndex);
    }

    // ordered
    if (reliability == PacketReliability::UnReliableSequenced || reliability == PacketReliability::ReliableSequenced || reliability == PacketReliability::ReliableOrdered || reliability == PacketReliability::ReliableOrderedWithAck) {
        bs.read(orderingIndex);
        bs.read(orderingChannel);
    }
    else {
        orderingChannel = 0;
    }

    if (splitPacket) {
        bs.read(splitPacketId);
        bs.read(splitPacketIndex);
        bs.read(splitPacketCount);
    }
    else {
        splitPacketCount = 0;

        // only the count is used to know if split, so set valeus on these to try detect incorrect usage.
#if X_DEBUG
        splitPacketId = std::numeric_limits<decltype(splitPacketId)>::max();
        splitPacketIndex = std::numeric_limits<decltype(splitPacketIndex)>::max();
#endif // !X_DEBUG
    }

    // check for corruption.

    // clang-format off

    X_WARNING_DIAG_PUSH
    X_DISABLE_WARNING_DIAG(tautological-constant-out-of-range-compare)

    // clang-format on

    if (bits == 0 || reliability > PacketReliability::ENUM_COUNT || orderingChannel >= MAX_ORDERED_STREAMS) {
        return false;
    }
    if (hasSplitPacket() && splitPacketIndex >= splitPacketCount) {
        return false;
    }
    X_WARNING_DIAG_POP

    allocData(bits);

    X_ASSERT(dataBitLength == bits, "bit count not set correct")(dataBitLength, bits);

    pData[core::bitUtil::bitsToBytes(bits) - 1] = 0; // zero last bit, as we may not have full byte.

    bs.readBitsAligned(pData, bits);
    return true;
}

void ReliablePacket::assignPropertiesExcData(const ReliablePacket* pOth)
{
    reliableMessageNumber = pOth->reliableMessageNumber;
    orderingIndex = pOth->orderingIndex;
    sequencingIndex = pOth->sequencingIndex;

    reliability = pOth->reliability;
    priority = pOth->priority;
    sendAttemps = 0;

    orderingChannel = pOth->orderingChannel;
    splitPacketId = pOth->splitPacketId;
    splitPacketIndex = pOth->splitPacketIndex;
    splitPacketCount = pOth->splitPacketCount;

    creationTime = pOth->creationTime;
    retransmissionTime = pOth->retransmissionTime;
    nextActionTime = pOth->nextActionTime;
}

PacketReliability::Enum ReliablePacket::reliabilityWithoutAck(void) const
{
    switch (reliability) {
        case PacketReliability::UnReliableWithAck:
            return PacketReliability::UnReliable;
        case PacketReliability::ReliableWithAck:
            return PacketReliability::Reliable;
        case PacketReliability::ReliableOrderedWithAck:
            return PacketReliability::ReliableOrdered;

        default:
            return reliability;
    }
}

// ----------------------------------------------------------

ReliabilityLayer::ReliabilityLayer(NetVars& vars,
    core::MemoryArenaBase* arena,
    core::MemoryArenaBase* packetDataArena,
    core::MemoryArenaBase* packetPool) :
    vars_(vars),
    arena_(arena),
    packetDataArena_(packetDataArena),
    packetPool_(packetPool),
    MTUSize_(0),
    orderingQueues_{{X_PP_REPEAT_COMMA_SEP(16, arena)}},
    outGoingPackets_(arena),
    recivedPackets_(arena),
    dataGramHistory_(arena),
    dataGramHistoryPopCnt_(0),
    recivedPacketBaseIdx_(0),
    recivedPacketQueue_(arena),
    incomingAcks_(arena),
    naks_(arena),
    acks_(arena),
    reliableMessageNumberIdx_(0),
    dagramSeqNumber_(0),

    splitPacketId_(0),
    splitPacketChannels_(arena),
    packetsThisFrame_(arena),
    packetsThisFrameBoundaries_(arena),
    connectionDead_(false),
    bps_{
        X_PP_REPEAT_COMMA_SEP(7, arena)},
    bytesInReSendBuffers_(0),
    msgInReSendBuffers_(0),
    delayedPackets_(arena)
{
    outGoingPackets_.reserve(128);
    recivedPackets_.reserve(128);

    // fifo doubles in size, so if we allocate one more than size we limit to.
    // it won't waste memory.
    dataGramHistory_.reserve(REL_DATAGRAM_HISTORY_LENGTH + 1);

    packetsThisFrame_.setGranularity(64);
    packetsThisFrameBoundaries_.setGranularity(64);

    resendBuf_.fill(nullptr);
}

ReliabilityLayer::~ReliabilityLayer()
{
    free();
}

void ReliabilityLayer::free(void)
{
    reset(MTUSize_);

    outGoingPackets_.free();
    recivedPackets_.free();
    dataGramHistory_.free();

    incomingAcks_.free();
    naks_.free();
    acks_.free();

    for (auto& orderQue : orderingQueues_) {
        orderQue.free();
    }

    for (uint32_t i = 0; i < NetStatistics::Metric::ENUM_COUNT; i++) {
        bps_[i].free();
    }

    splitPacketChannels_.free();

    packetsThisFrame_.free();
    packetsThisFrameBoundaries_.free();
}

void ReliabilityLayer::reset(int32_t MTUSize)
{
    clearPacketQueues();
    freeResendList();

    MTUSize_ = MTUSize;

    auto timeNow = gEnv->pTimer->GetTimeNowReal();

    timeLastDatagramArrived_ = timeNow;
    lastBSPUpdate_ = timeNow;
    orderedWriteIndex_.fill(0);
    sequencedWriteIndex_.fill(0);
    orderedReadIndex_.fill(0);
    highestSequencedReadIndex_.fill(0);
    orderingQueueIndexOffset_.fill(0);

    for (auto& orderQue : orderingQueues_) {
        orderQue.clear();
    }

    outGoingPackets_.clear();
    recivedPackets_.clear();
    dataGramHistory_.clear();
    dataGramHistoryPopCnt_ = 0;
    recivedPacketBaseIdx_ = 0;

    incomingAcks_.clear();
    naks_.clear();
    acks_.clear();

    for (auto* pSplitChannel : splitPacketChannels_) {
        freeSplitPacketChannel(pSplitChannel);
    }
    splitPacketChannels_.clear();
    reliableMessageNumberIdx_ = 0;
    dagramSeqNumber_ = 0;
    splitPacketId_ = 0;

    connectionDead_ = false;

    msgInSendBuffers_.fill(0);
    bytesInSendBuffers_.fill(0);

    for (uint32_t i = 0; i < NetStatistics::Metric::ENUM_COUNT; i++) {
        bps_[i].reset();
    }

    msgInSendBuffers_.fill(0);
    bytesInSendBuffers_.fill(0);

    bytesInReSendBuffers_ = 0;
    msgInReSendBuffers_ = 0;

    resendBuf_.fill(nullptr);

#if X_DEBUG && 0
    // lets overflow soon, aint nobody got time for waiting.
    dagramSeqNumber_ = std::numeric_limits<decltype(dagramSeqNumber_)>::max() - 1024;
    dataGramHistoryPopCnt_ = dagramSeqNumber_;
#endif // !dagramSeqNumber_
}

void ReliabilityLayer::clearPacketQueues(void)
{
    while (outGoingPackets_.isNotEmpty()) {
        freePacket(outGoingPackets_.peek());
        outGoingPackets_.pop();
    }

    while (recivedPackets_.isNotEmpty()) {
        freePacket(recivedPackets_.peek());
        recivedPackets_.pop();
    }
}

bool ReliabilityLayer::send(const uint8_t* pData, const BitSizeT lengthBits, core::TimeVal time, uint32_t mtuSize,
    PacketPriority::Enum priority, PacketReliability::Enum reliability, uint8_t orderingChannel, SendReceipt receipt, bool ownData)
{
    X_ASSERT_NOT_NULL(pData);
    X_ASSERT(lengthBits > 0, "Must call with alreast some bits")();

    auto lengthBytes = core::bitUtil::bitsToBytes(lengthBits);

    X_LOG0_IF(vars_.debugEnabled(), "NetRel", "\"%s\" msg added. Pri: \"%s\" size: ^5%" PRIu32 "^7 numbits: ^5%" PRIu32,
        PacketReliability::ToString(reliability), PacketPriority::ToString(priority), lengthBytes, lengthBits);

    // can this data fit in single packet?
    const size_t maxDataSizeBytes = maxDataGramSize() - ReliablePacket::getMaxHeaderLength();
    const bool splitRequired = lengthBytes > maxDataSizeBytes;

    ReliablePacket* pPacket = allocPacket();
    pPacket->reliableMessageNumber = 0;
    pPacket->creationTime = time;
    pPacket->dataBitLength = lengthBits;
    pPacket->priority = priority;
    pPacket->reliability = reliability;
    pPacket->sendAttemps = 0;
    pPacket->sendReceipt = receipt;

    if (ownData) {
        // can skip copy as we now own the buffer.
        pPacket->setArena(packetDataArena_);
        pPacket->pData = const_cast<uint8_t*>(pData);
        pPacket->dataBitLength = lengthBits;
    }
    else {
        // must allocate buffer and copy.
        pPacket->allocData(lengthBits);
        std::memcpy(pPacket->pData, pData, lengthBytes);
    }

    if (splitRequired) {
        // upgrade reliability as we gonna be sending in diffrent datagrams.
        if (reliability == PacketReliability::UnReliable) {
            pPacket->reliability = PacketReliability::Reliable;
        }
        else if (reliability == PacketReliability::UnReliableSequenced) {
            pPacket->reliability = PacketReliability::ReliableSequenced;
        }
        else if (reliability == PacketReliability::UnReliableWithAck) {
            pPacket->reliability = PacketReliability::ReliableWithAck;
        }

        if (pPacket->reliability != reliability) {
            X_LOG0_IF(vars_.debugEnabled(), "NetRel", "Upgraded reliability from \"%s\" to \"%s\"",
                PacketReliability::ToString(reliability), PacketReliability::ToString(pPacket->reliability));
        }
    }
    else {
        pPacket->splitPacketId = 0;
        pPacket->splitPacketIndex = 0;
        pPacket->splitPacketCount = 0;
    }

    // sequenced packets
    if (reliability == PacketReliability::ReliableSequenced || reliability == PacketReliability::UnReliableSequenced) {
        pPacket->orderingChannel = orderingChannel;
        pPacket->orderingIndex = orderedWriteIndex_[orderingChannel];
        pPacket->sequencingIndex = sequencedWriteIndex_[orderingChannel]++;
    }
    else if (reliability == PacketReliability::ReliableOrdered || reliability == PacketReliability::ReliableOrderedWithAck) {
        pPacket->orderingChannel = orderingChannel;
        pPacket->orderingIndex = orderedWriteIndex_[orderingChannel]++;

        sequencedWriteIndex_[orderingChannel] = 0;
    }
    else {
        pPacket->orderingChannel = std::numeric_limits<decltype(pPacket->orderingChannel)>::max();
        pPacket->orderingIndex = std::numeric_limits<OrderingIndex>::max();
        pPacket->sequencingIndex = std::numeric_limits<OrderingIndex>::max();
    }

    bps_[NetStatistics::Metric::BytesPushed].add(time, lengthBytes);

    if (splitRequired) {
        return splitPacket(pPacket);
    }

    outGoingPackets_.push(pPacket);

    ++msgInSendBuffers_[priority];
    bytesInSendBuffers_[priority] += lengthBytes;
    return true;
}

bool ReliabilityLayer::recv(uint8_t* pData, const size_t length, NetSocket& socket,
    SystemAddressEx& systemAddress, core::TimeVal time, uint32_t mtuSize)
{
    X_LOG0_IF(vars_.debugDatagramEnabled(), "NetRel", "Recived datagram size: ^5%" PRIuS "^7 numbits: ^5%" PRIuS, length, core::bitUtil::bytesToBits(length));

    // last time we got data.
    timeLastDatagramArrived_ = gEnv->pTimer->GetTimeNowReal();

    bps_[NetStatistics::Metric::ActualBytesReceived].add(time, length);

    core::FixedBitStreamNoneOwning bs(pData, pData + length, true);

    DatagramHdr dgh;
    dgh.fromBitStream(bs);

    if (vars_.debugDatagramEnabled()) {
        DatagramFlags::Description Dsc;
        X_LOG0("NetRel", "DataGram number: ^5%" PRIu16 "^7 Flags: \"%s\"", dgh.number, dgh.flags.ToString(Dsc));
    }

    if (dgh.flags.IsSet(DatagramFlag::Ack)) {
        // ack, ack, ack!
        incomingAcks_.clear();

        if (!incomingAcks_.fromBitStream(bs)) {
            X_ERROR("NetRel", "Failed to process incomming ack's");
            return false;
        }

        for (auto& ackRange : incomingAcks_) {
            // we want to mark all these messages as recived so we don't resend like a pleb.
            X_LOG0_IF(vars_.debugAckEnabled(), "NetRel", "Act Range: ^5%" PRIu16 " ^7-^5 % " PRIu16, ackRange.min, ackRange.max);

            for (DataGramSequenceNumber dataGramIdx = ackRange.min;
                 // handle wrap by checking above min, saves load of up and down casting
                 dataGramIdx >= ackRange.min && dataGramIdx <= ackRange.max;
                 dataGramIdx++) {
                // get the info for this data gram and mark all the packets it contained as sent.
                DataGramHistory* pHistory = getDataGramHistory(dataGramIdx);
                if (!pHistory) {
                    X_WARNING("NetRel", "Failed to get dataGram history for idx: %" PRIu16 " will result in resend", dataGramIdx);
                    continue;
                }

                // mark each as sent.
                for (auto msgNum : pHistory->messagenumbers) {
                    removePacketFromResendList(msgNum);
                }

                pHistory->messagenumbers.clear();
            }
        }
    }
    else if (dgh.flags.IsSet(DatagramFlag::Nack)) {
        // nick nack pady wack
        X_ALIGNED_SYMBOL(char allocBuf[core::bitUtil::RoundUpToMultiple(MAX_MTU_SIZE, 256u) + 512], 16) = {};

        core::StackAllocator allocator(allocBuf, allocBuf + sizeof(allocBuf));

        typedef core::MemoryArena<
            core::StackAllocator,
            core::SingleThreadPolicy,
            core::NoBoundsChecking,
            core::NoMemoryTracking,
            core::NoMemoryTagging>
            StackArena;

        // we don't get many of these sluts.
        StackArena arena(&allocator, "NackArena");
        DataGramNumberRangeList incomingNacks(&arena);

        if (!incomingNacks.fromBitStream(bs)) {
            X_ERROR("NetRel", "Failed to process incomming nacks's");
            return false;
        }

        for (auto& nackRange : incomingNacks) {
            // mark all the msg's for resend immediatly.
            X_LOG0_IF(vars_.debugNackEnabled(), "NetRel", "Nact Range: ^5%" PRIu16 " ^7-^5 % " PRIu16, nackRange.min, nackRange.max);

            for (DataGramSequenceNumber dataGramIdx = nackRange.min; dataGramIdx <= nackRange.max; dataGramIdx++) {
                DataGramHistory* pHistory = getDataGramHistory(dataGramIdx);
                if (!pHistory) {
                    X_WARNING("NetRel", "Failed to get dataGram history for idx: %" PRIu16 " will result in resend", dataGramIdx);
                    continue;
                }

                // mark each as sent.
                for (auto msgNum : pHistory->messagenumbers) {
                    const auto resendBufIdx = resendBufferIdxForMsgNum(msgNum);
                    ReliablePacket* pPacket = resendBuf_[resendBufIdx];

                    X_LOG0_IF(vars_.debugNackEnabled(), "NetRel", "Nack msgNum: ^5 % " PRIu16, msgNum);

                    if (pPacket) {
                        if (pPacket->nextActionTime.GetValue() != 0) {
                            pPacket->nextActionTime = time;
                        }
                    }
                }

                pHistory->messagenumbers.clear();
            }
        }
    }
    else {
        // I GOT IT !
        // we ack for unreliable also, but never resend if no ack recived,
        addAck(dgh.number);

        // we can have multiple goat packets in a single MTU.
        // so we keep processing the bitStream untill it's empty.

        ReliablePacket* pPacket = packetFromBS(bs, time);
        while (pPacket) {
            auto result = prcoessIncomingPacket(pPacket, time);
            if (result == ProcessResult::Ok) {
                // do nothing..
                // it was either pushed to the que for us
                // OR is been buffred untill it's in order.
            }
            else if (result == ProcessResult::Ignored) {
                ignorePacket(pPacket, time);
            }
            else {
                X_ASSERT_UNREACHABLE();
            }

            // trailing bits.
            if (bs.size() < 8) {
                bs.alignReadToByteBoundry();
                break;
            }

            pPacket = packetFromBS(bs, time);
        }

        X_ASSERT(bs.isEos(), "Unprocessed bytes")(bs.isEos(), bs.size(), bs.capacity());
    }

    return true;
}

ReliabilityLayer::ProcessResult::Enum ReliabilityLayer::prcoessIncomingPacket(ReliablePacket*& pPacketInOut, core::TimeVal time)
{
    ReliablePacket* pPacket = pPacketInOut;

    X_ASSERT(!pPacket->isAckRequired(), "Ack should be dropped from reliability type before sending")(pPacket->reliability);

    // if ordered range check channel to prevent a crash.
    if (pPacket->isOrderedOrSequenced()) {
        if (pPacket->orderingChannel >= MAX_ORDERED_STREAMS) {
            // bitch who you think you are.
            X_ERROR("NetRel", "Recived packet with invalid channel: %" PRIu16 " ignoring.", pPacket->orderingChannel);
            return ProcessResult::Ignored;
        }
    }

    if (pPacket->isReliable()) {
        // intentional unsigned overflow.
        DataGramSequenceNumber holeSize = static_cast<DataGramSequenceNumber>(pPacket->reliableMessageNumber - recivedPacketBaseIdx_);

        if (holeSize == 0) {
            // we expected this packet.
            ++recivedPacketBaseIdx_;
            // if we had a hole at start it's now filled.
            if (recivedPacketQueue_.isNotEmpty()) {
                recivedPacketQueue_.pop();
            }
        }
        else if (holeSize > std::numeric_limits<decltype(holeSize)>::max() / 2) // is negative?
        {
            // duplicate packet.
            X_LOG0_IF(vars_.debugIgnoredEnabled(), "NetRel", "Recived duplicate packet. (hole neg)");
            return ProcessResult::Ignored;
        }
        else if (holeSize < recivedPacketQueue_.size()) // reviced a packet that is higer than base, but lower than highest packet we seen.
        {
            if (recivedPacketQueue_[holeSize]) {
                // this hole is already filled so it's duplicate.
                X_LOG0_IF(vars_.debugIgnoredEnabled(), "NetRel", "Recived duplicate packet. (hole fill)");
                return ProcessResult::Ignored;
            }

            recivedPacketQueue_[holeSize] = true;
        }
        else {
            // we got a packet higer than base and higer than we seen before.
            // impose some sort of limit on max hole, otherwise memory for hole logic grow quite large.
            if (holeSize > REL_MAX_RECIVE_HOLE) {
                X_LOG0_IF(vars_.debugIgnoredEnabled(), "NetRel", "Recived packet that has a message number greater than recive hole");
                return ProcessResult::Ignored;
            }

            // need to example the que so it's the size of new hole.
            // marking new ones as holes.
            while (holeSize > recivedPacketQueue_.size()) {
                recivedPacketQueue_.push(false);
            }

            // mark last one as got aka this packet.
            recivedPacketQueue_.push(true);

            X_ASSERT(recivedPacketQueue_.size() < std::numeric_limits<decltype(holeSize)>::max(), "Que is bigger than type range")();
        }

        // pop any complete ones from base, moving base index up.
        while (recivedPacketQueue_.isNotEmpty() && recivedPacketQueue_.peek()) {
            recivedPacketQueue_.pop();
            ++recivedPacketBaseIdx_;
        }

        if (recivedPacketQueue_.capacity() > REL_RECIVE_HOLE_SHRINK_THRESHOLD) {
            // regain some memory if we've just come out of a large hole.
            const auto unusedSpace = recivedPacketQueue_.capacity() - recivedPacketQueue_.size();
            if (unusedSpace > recivedPacketQueue_.size() * 4) {
                X_LOG0_IF(vars_.debugIgnoredEnabled(), "NetRel", "Shrinking packet queue");
                recivedPacketQueue_.shrinkToFit();
            }
        }
    }

    if (pPacket->hasSplitPacket()) {
        // we need to keep track of these untill we have all the packets
        pPacket = addIncomingSplitPacket(pPacket, time);
        if (!pPacket) {
            // still more packets.
            return ProcessResult::Ok;
        }

        // if here we have a split packets that's been rebuilt into original packet.
        // all split packets should of been cleaned up and data copyied into this new packet.

        // we can't return here, otherwise sequenced or ordered packets that where split
        // won't get handled correct.

        // update the inOut pointer so if we ignore correct packet is cleaned up.
        pPacketInOut = pPacket;
    }

    if (pPacket->isOrderedOrSequenced()) {
        const auto channel = pPacket->orderingChannel;

        if (pPacket->orderingIndex == orderedReadIndex_[channel]) {
            // ordered / sequenced handling for when ordering index matches expected.
            if (pPacket->isSequenced()) {
                // we ignore any packets that are older than we have seen.
                if (isOlderPacket(pPacket->sequencingIndex, highestSequencedReadIndex_[channel])) {
                    X_LOG0_IF(vars_.debugIgnoredEnabled(), "NetRel", "Recived duplicate packet. (older sequenced)");
                    return ProcessResult::Ignored;
                }

                highestSequencedReadIndex_[channel] = pPacket->sequencingIndex + 1;
            }
            else // ordered.
            {
                // this packet is allowed to be sent to user.
                addPacketToRecivedQueue(pPacket, time);

                ++orderedReadIndex_[channel];
                highestSequencedReadIndex_[channel] = 0; // when we move ordering index, sequenced is reset.

                auto& orderedQueue = orderingQueues_[channel];
                while (orderedQueue.isNotEmpty() && orderedQueue.peek().pPacket->orderingIndex == orderedReadIndex_[channel]) {
                    auto* pBufferedPacket = orderedQueue.peek().pPacket;
                    orderedQueue.pop();

                    if (pBufferedPacket->isSequenced()) {
                        highestSequencedReadIndex_[channel] = pBufferedPacket->sequencingIndex;
                    }
                    else {
                        ++orderedReadIndex_[channel];
                    }

                    addPacketToRecivedQueue(pBufferedPacket, time);
                }

                // done...
                return ProcessResult::Ok;
            }
        }
        else if (!isOlderPacket(pPacket->orderingIndex, orderedReadIndex_[channel])) {
            // this packet comes later than the one we want next.
            // so store it untill we can post them in order.
            X_LOG0_IF(vars_.debugEnabled(), "NetRel", "Recived out of order order/sequenced packet.");

            auto& orderedQueue = orderingQueues_[channel];
            if (orderedQueue.isEmpty()) {
                orderingQueueIndexOffset_[channel] = orderedReadIndex_[channel];
            }

            // work out how many
            const WeightType orderChanelSpacing = std::numeric_limits<WeightType>::max() / std::numeric_limits<OrderingIndex>::max();
            static_assert(orderChanelSpacing > std::numeric_limits<OrderingIndex>::max(), "Can't store max potentially sequences between ordered.");

            WeightType orderedHoleCount = pPacket->orderingIndex - orderingQueueIndexOffset_[channel];
            WeightType weight = orderedHoleCount * orderChanelSpacing;

            if (pPacket->isSequenced()) {
                weight += pPacket->sequencingIndex;
            }
            else {
                // this comes out after all the sequenced ones on this channel.
                weight += (orderChanelSpacing - 1);
            }

            orderedQueue.emplace(weight, pPacket);
            return ProcessResult::Ok;
        }
        else {
            // this packet is older than what we expecting
            // we can ignore it.
            X_LOG0_IF(vars_.debugIgnoredEnabled(), "NetRel", "Recived duplicate packet. (ordered older)");
            return ProcessResult::Ignored;
        }
    }

    addPacketToRecivedQueue(pPacket, time);
    return ProcessResult::Ok;
}

void ReliabilityLayer::ignorePacket(ReliablePacket* pPacket, core::TimeVal time)
{
    const size_t packetDataByteLength = core::bitUtil::bitsToBytes(pPacket->dataBitLength);
    bps_[NetStatistics::Metric::BytesRecivedIgnored].add(time, packetDataByteLength);

    X_WARNING_IF(vars_.debugIgnoredEnabled(), "NetRel", "Packet ignored");

    freePacket(pPacket);
}

void ReliabilityLayer::addPacketToRecivedQueue(ReliablePacket* pPacket, core::TimeVal time)
{
    const size_t byteLength = core::bitUtil::bitsToBytes(pPacket->dataBitLength);
    bps_[NetStatistics::Metric::BytesRecivedProcessed].add(time, byteLength);

    recivedPackets_.push(pPacket);
}

void ReliabilityLayer::update(core::FixedBitStreamBase& bs, NetSocket& socket, SystemAddressEx& systemAddress, int32_t MTUSize,
    core::TimeVal time)
{
    // delay list, these are packets that have already been sent, but are having artifical latency added.
    if (delayedPackets_.isNotEmpty()) {
        while (delayedPackets_.isNotEmpty() && delayedPackets_.peek().sendTime < time) {
            // sendy wendy.
            auto& pDelayPacket = delayedPackets_.peek();

            SendParameters sp;
            sp.pData = pDelayPacket.data.get();
            sp.length = safe_static_cast<int32_t>(pDelayPacket.sizeInBytes);
            sp.systemAddress = systemAddress;
            pDelayPacket.pSocket->send(sp);

            delayedPackets_.pop();
        }
    }

    bs.reset();

    if ((lastBSPUpdate_ + core::TimeVal::fromMS(1000)) < time) {
        for (uint32_t i = 0; i < NetStatistics::Metric::ENUM_COUNT; i++) {
            bps_[i].update(time);
        }

        lastBSPUpdate_ = time;
    }

    const int32_t bitsPerSecondLimit = vars_.connectionBSPLimit();
    X_UNUSED(bitsPerSecondLimit);

    // HELLLLOOOO annnnyboddy.
    if (hasTimedOut(time)) {
        X_ERROR("NetRel", "Connection timed out");
        connectionDead_ = true;
        return;
    }

    // send acks.
    if (acks_.isNotEmpty()) {
        sendACKs(socket, bs, systemAddress, time);
    }

    // send naks.
    if (naks_.isNotEmpty()) {
        sendNAKs(socket, bs, systemAddress, time);
    }

    X_ASSERT(packetsThisFrame_.isEmpty(), "Should of been previously cleared")();
    X_ASSERT(packetsThisFrameBoundaries_.isEmpty(), "Should of been previously cleared")();

    // resend packets.
    if (!isResendListEmpty()) {
        while (1) {
            // look at all the packets in resend list.
            // and build dataGrams untill we reach a packet who's nextaction time has not yet been reached.
            size_t packetCntBase = packetsThisFrame_.size();
            size_t currentDataGramSizeBits = 0;
            const size_t maxDataGramSizeBits = maxDataGramSizeExcHdrBits();

            while (!isResendListEmpty() && (packetCntBase - packetsThisFrame_.size()) < MAX_REL_PACKETS_PER_DATAGRAM) {
                ReliablePacket* pPacket = resendList_.head();
                if (pPacket->nextActionTime > time) {
                    break;
                }

                // will it fit ;) ?
                const size_t byteLength = core::bitUtil::bitsToBytes(pPacket->dataBitLength);
                const size_t totalBitSize = pPacket->getHeaderLengthBits() + pPacket->dataBitLength;

                if ((currentDataGramSizeBits + totalBitSize) > maxDataGramSizeBits) {
                    break;
                }

                // remove from head, and place back in at tail.
                // so the list stays sorted based on action time.
                movePacketToTailOfResendList(pPacket);

                // add packet.
                currentDataGramSizeBits += totalBitSize;
                packetsThisFrame_.emplace_back(pPacket);

                X_LOG0_IF(vars_.debugEnabled(), "NetRel", "Resending packet: ^5%" PRIu32 "^7 attempt: ^5%" PRIu8,
                    pPacket->reliableMessageNumber, pPacket->sendAttemps);

                // resending.
                bps_[NetStatistics::Metric::BytesResent].add(time, byteLength);

                ++pPacket->sendAttemps;

                // TODO.
                pPacket->retransmissionTime = core::TimeVal::fromMS(2000);
                pPacket->nextActionTime = time + pPacket->retransmissionTime;
            }

            // datagram is empty?
            if (currentDataGramSizeBits == 0) {
                break;
            }

            // add datagram.
            packetsThisFrameBoundaries_.emplace_back(safe_static_cast<int32_t>(packetsThisFrame_.size()));
            if (packetsThisFrameBoundaries_.size() == REL_DATAGRAM_HISTORY_LENGTH) {
                break;
            }
        }
    }

    // send new packets.
    if (!outGoingPackets_.isEmpty()) {
        while (!isResendBufferFull()) {
            // fill a packet.
            size_t currentReliablePackets = 0;
            size_t currentDataGramSizeBits = 0;
            const size_t maxDataGramSizeBits = maxDataGramSizeExcHdrBits();

            while (outGoingPackets_.isNotEmpty() && !isResendBufferFull() && currentReliablePackets < MAX_REL_PACKETS_PER_DATAGRAM) {
                ReliablePacket* pPacket = outGoingPackets_.peek();

                // meow.
                X_ASSERT_NOT_NULL(pPacket->pData);
                X_ASSERT(pPacket->dataBitLength > 0, "Invalid data length")();

                const size_t byteLength = core::bitUtil::bitsToBytes(pPacket->dataBitLength);
                const size_t totalBitSize = pPacket->getHeaderLengthBits() + pPacket->dataBitLength;

                if ((currentDataGramSizeBits + totalBitSize) > maxDataGramSizeBits) {
                    // check this is not first packet. otherwise it will never fit. ;)
                    X_ASSERT(currentDataGramSizeBits > 0, "Packet is too big to fit in single datagram")(currentDataGramSizeBits, totalBitSize);
                    break;
                }

                outGoingPackets_.pop();

                // add packet.
                currentDataGramSizeBits += totalBitSize;
                packetsThisFrame_.emplace_back(pPacket);

                // stats
                --msgInSendBuffers_[pPacket->priority];
                bytesInSendBuffers_[pPacket->priority] -= byteLength;

                bps_[NetStatistics::Metric::BytesSent].add(time, byteLength);
                // ~

                ++pPacket->sendAttemps;

                if (pPacket->isReliable()) {
                    pPacket->reliableMessageNumber = reliableMessageNumberIdx_;
                    pPacket->retransmissionTime = core::TimeVal::fromMS(2000);
                    pPacket->nextActionTime = time + pPacket->retransmissionTime;

                    const auto resendBufIdx = resendBufferIdxForMsgNum(pPacket->reliableMessageNumber);
                    X_ASSERT(resendBuf_[resendBufIdx] == nullptr, "Resent buffer has valid data already in it")();
                    resendBuf_[resendBufIdx] = pPacket;

                    ++reliableMessageNumberIdx_;

                    insertPacketToResendList(pPacket);

                    // stats
                    bytesInReSendBuffers_ += byteLength;
                    ++msgInReSendBuffers_;
                    // ~

                    ++currentReliablePackets;
                }
                else if (pPacket->reliability == PacketReliability::UnReliableWithAck) {
                    // ack me!
                }
                else {
                    // ...
                }
            }

            // datagram is empty?
            if (currentDataGramSizeBits == 0) {
                break;
            }

            // add datagram.
            packetsThisFrameBoundaries_.emplace_back(safe_static_cast<int32_t>(packetsThisFrame_.size()));
            if (packetsThisFrameBoundaries_.size() == REL_DATAGRAM_HISTORY_LENGTH) {
                break;
            }
        }

        if (outGoingPackets_.isNotEmpty() && isResendBufferFull()) {
            X_LOG0_IF(vars_.debugEnabled(), "NetRel", "Resend buffer is full, postponing send of %" PRIuS " packets", outGoingPackets_.size());
        }
    }

    // dispatch all the datagrams. (if any)
    for (size_t i = 0; i < packetsThisFrameBoundaries_.size(); i++) {
        int32_t begin, end;

        if (i == 0) {
            begin = 0;
            end = packetsThisFrameBoundaries_[i];
        }
        else {
            begin = packetsThisFrameBoundaries_[i - 1];
            end = packetsThisFrameBoundaries_[i];
        }

        X_ASSERT(end >= begin, "Invalid range")(begin, end);
        X_ASSERT(bs.capacity() >= maxDataGramSize(), "Provided Bs can't fit max dataGram")(bs.capacity(), maxDataGramSize());

        const size_t numPackets = static_cast<size_t>(end - begin);

        // here we pack the packet range into BS.
        bs.reset();

        DatagramHdr dgh;
        dgh.number = dagramSeqNumber_++;
        dgh.flags.Clear();
        dgh.writeToBitStream(bs);

        X_ASSERT(bs.size() == dataGramHdrSizeBits(), "Invalid size logic")(bs.size(), dataGramHdrSizeBits());
        X_LOG0_IF(vars_.debugDatagramEnabled(), "NetRel", "Sending DataGram number: ^5%" PRIu16 "^7 numPackets: ^5%" PRIuS, dgh.number, numPackets);

        // we create dataGram history even for dataGrams not containing reliable packets.
        // so that the datagram indexes match up.
        // if we are sending alot of unreliable packets it may be best to change logic of ack
        // to support datagram history with gaps.
        DataGramHistory* pHistory = createDataGramHistory(dgh.number, time);

        while (begin < end) {
            ReliablePacket* pPacket = packetsThisFrame_[begin];

            pPacket->writeToBitStream(bs);

            // only if reliable add to history.
            if (pPacket->isReliable()) {
                pHistory->messagenumbers.append(pPacket->reliableMessageNumber);
            }

            ++begin;
        }

        // send it.
        sendBitStream(socket, bs, systemAddress, time);
    }

    // we delete any none reliable we sent this frame.
    for (auto* pPacket : packetsThisFrame_) {
        if (!pPacket->isReliable()) {
            freePacket(pPacket);
        }
    }

    packetsThisFrame_.clear();
    packetsThisFrameBoundaries_.clear();
}

// called from peer to get recived packets back from ReliabilityLayer.
bool ReliabilityLayer::recive(PacketData& dataOut)
{
    if (recivedPackets_.isEmpty()) {
        return false;
    }

    ReliablePacket* pPacket = recivedPackets_.peek();
    dataOut.setdata(pPacket->pData, pPacket->dataBitLength, pPacket->getArena());
    recivedPackets_.pop();

    // release ownership...
    pPacket->pData = nullptr;
    pPacket->dataBitLength = 0;
    X_ASSERT(pPacket->dataType != ReliablePacket::DataType::Ref, "Should not have refrenced data for recived packets")();

    freePacket(pPacket);
    return true;
}

bool ReliabilityLayer::hasTimedOut(core::TimeVal time)
{
    core::TimeVal elapsed = time - timeLastDatagramArrived_;

    if (elapsed > core::TimeVal::fromMS(vars_.defaultTimeoutMS())) {
        return true;
    }

    return false;
}

void ReliabilityLayer::sendACKs(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAddressEx& systemAddress, core::TimeVal time)
{
    // we want to pack all the acks into packets.
    const BitSizeT maxPacketBits = maxDataGramSizeExcHdrBits();

    while (acks_.isNotEmpty()) {
        bs.reset();

        DatagramHdr dgh;
        dgh.number = 0;
        dgh.flags.Set(DatagramFlag::Ack);
        dgh.writeToBitStream(bs);

        // return instead of infinate loop, but if this happens dunno how we'd recover :Z
        // but it never 'should' happen.
        if (acks_.writeToBitStream(bs, maxPacketBits, true) == 0) {
            X_ERROR("NetRel", "Failed to write any ack's to stream");
            return;
        }

        // Weeeeeee!
        sendBitStream(socket, bs, systemAddress, time);
    }
}

void ReliabilityLayer::sendNAKs(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAddressEx& systemAddress, core::TimeVal time)
{
    // we want to pack all the acks into packets.
    const BitSizeT maxPacketBits = maxDataGramSizeExcHdrBits();

    while (naks_.isNotEmpty()) {
        bs.reset();

        DatagramHdr dgh;
        dgh.number = 0;
        dgh.flags.Set(DatagramFlag::Nack);
        dgh.writeToBitStream(bs);

        if (naks_.writeToBitStream(bs, maxPacketBits, true) == 0) {
            X_ERROR("NetRel", "Failed to write any nack's to stream");
            return;
        }

        sendBitStream(socket, bs, systemAddress, time);
    }
}

void ReliabilityLayer::sendBitStream(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAddressEx& systemAddress, core::TimeVal time)
{
    if (vars_.artificalNetworkEnabled()) {
        float percent = vars_.artificalPacketLoss();
        float randVal = gEnv->xorShift.randRange(0.f, 100.f);

        if (randVal < percent) {
            X_LOG0_IF(vars_.debugEnabled() > 2, "NetRel", "Dropping packet");
            return;
        }

        // pingy.
        if (vars_.artificalPing() || vars_.artificalPingVariance()) {
            uint32_t totalMsDelay = vars_.artificalPing();

            // dash of spice!
            if (vars_.artificalPingVariance()) {
                uint32_t randomVariance = gEnv->xorShift.randRange(0u, vars_.artificalPingVariance());
                totalMsDelay += randomVariance;
            }

            // delay could be zero and random variance zero.
            if (totalMsDelay) {
                core::TimeVal delaySendTime = time + core::TimeVal::fromMS(totalMsDelay);

                uint8_t* pDataCopy = X_NEW_ARRAY_ALIGNED(uint8_t, bs.sizeInBytes(), packetDataArena_, "DelayPacketData", 16);
                std::memcpy(pDataCopy, bs.data(), bs.sizeInBytes());

                delayedPackets_.emplace(
                    &socket,
                    delaySendTime,
                    bs.sizeInBytes(),
                    pDataCopy,
                    packetDataArena_);

                return;
            }
        }
    }

    bps_[NetStatistics::Metric::ActualBytesSent].add(time, bs.sizeInBytes());

    SendParameters sp;
    sp.setData(bs);
    sp.systemAddress = systemAddress;
    socket.send(sp);
}

ReliablePacket* ReliabilityLayer::packetFromBS(core::FixedBitStreamBase& bs, core::TimeVal time)
{
    // meow.
    if (bs.isEos()) {
        return nullptr;
    }

    ReliablePacket* pPacket = allocPacket();
    pPacket->creationTime = time;
    if (!pPacket->fromBitStream(bs)) {
        freePacket(pPacket);
        return nullptr;
    }

    return pPacket;
}

ReliablePacket* ReliabilityLayer::allocPacket(void)
{
    return X_NEW(ReliablePacket, packetPool_, "RelPacket")(packetDataArena_);
}

void ReliabilityLayer::freePacket(ReliablePacket* pPacket)
{
    pPacket->freeData();
    X_DELETE(pPacket, packetPool_);
}

SplitPacketChannel* ReliabilityLayer::allocSplicPacketChannel(void)
{
    return X_NEW(SplitPacketChannel, arena_, "PacketChannel")(arena_);
}

void ReliabilityLayer::freeSplitPacketChannel(SplitPacketChannel* pSPC)
{
    X_ASSERT_NOT_NULL(pSPC);

    for (auto* pPacket : pSPC->packets) {
        if (pPacket) // can be null if not complete.
        {
            pPacket->freeData();
            freePacket(pPacket);
        }
        else {
            X_ASSERT(!pSPC->haveAllPackets(), "We think we have all packets but one is null")(pSPC, pSPC->haveAllPackets());
        }
    }

    X_DELETE(pSPC, arena_);
}

bool ReliabilityLayer::splitPacket(ReliablePacket* pPacket)
{
    const auto lengthBytes = core::bitUtil::bitsToBytes(pPacket->dataBitLength);
    const size_t maxDataSizeBytes = maxDataGramSizeExcHdr() - ReliablePacket::getMaxHeaderLength();
    const bool splitRequired = lengthBytes > maxDataSizeBytes;

    X_ASSERT(splitRequired, "Called split when no split required")(splitRequired, lengthBytes, maxDataSizeBytes);

    pPacket->splitPacketCount = safe_static_cast<SplitPacketIndex>((lengthBytes - 1) / (maxDataSizeBytes) + 1);

    X_ASSERT(pPacket->splitPacketCount > 1, "Packet was not split")(pPacket->splitPacketCount, lengthBytes, maxDataSizeBytes);
    X_ASSERT((pPacket->splitPacketCount * maxDataSizeBytes) >= lengthBytes, "Split count can't represent packet data")(pPacket->splitPacketCount, maxDataSizeBytes, lengthBytes);

    core::HumanSize::Str sizeBuf;
    X_LOG0_IF(vars_.debugEnabled(), "NetRel", "Splitting packet of size %s into ^5%" PRIu16 "^7 packets",
        core::HumanSize::toString(sizeBuf, core::bitUtil::bitsToBytes(pPacket->dataBitLength)), pPacket->splitPacketCount);

    core::StopWatch timer;

    // temp array.
    core::Array<ReliablePacket*> packets(arena_);
    packets.resize(pPacket->splitPacketCount);

    SplitPacketId splitPacketId = splitPacketId_++;

    X_ASSERT(g_NetworkArena->isThreadSafe(), "Peer arena must be thread safe")(g_NetworkArena->isThreadSafe());

    DataRefrence* pRefData = X_NEW(DataRefrence, g_NetworkArena, "SplitPacketDataRef");
    pRefData->pData = pPacket->pData;

    for (SplitPacketIndex packetIdx = 0; packetIdx < pPacket->splitPacketCount; packetIdx++) {
        ReliablePacket* pSplitPacket = allocPacket();
        pSplitPacket->assignPropertiesExcData(pPacket);
        // some overrides
        pSplitPacket->sendAttemps = 0;
        pSplitPacket->splitPacketId = splitPacketId;
        pSplitPacket->splitPacketIndex = packetIdx;

        size_t offset = packetIdx * maxDataSizeBytes;
        size_t packetSizeBytes = core::Min(maxDataSizeBytes, lengthBytes - offset);

        // this packet refrences the data.
        pSplitPacket->dataType = ReliablePacket::DataType::Ref;
        pSplitPacket->pData = pPacket->pData + offset;
        pSplitPacket->pRefData = pRefData;

        if (packetIdx > 0) {
            pRefData->addReference();
        }

        if (packetSizeBytes == maxDataSizeBytes) {
            pSplitPacket->dataBitLength = safe_static_cast<BitSizeT>(core::bitUtil::bytesToBits(packetSizeBytes));
        }
        else {
            // work out trailing bit count.
            pSplitPacket->dataBitLength = pPacket->dataBitLength - safe_static_cast<BitSizeT>(core::bitUtil::bytesToBits(maxDataSizeBytes) * packetIdx);
        }

        packets[packetIdx] = pSplitPacket;
    }

    // push them all.
    const auto priority = pPacket->priority;
    for (auto* pSplitPacket : packets) {
        outGoingPackets_.push(pSplitPacket);

        ++msgInSendBuffers_[priority];
        bytesInSendBuffers_[priority] += core::bitUtil::bitsToBytes(pSplitPacket->dataBitLength);
    }

    X_LOG0_IF(vars_.debugEnabled(), "NetRel", "Splitpacket took: ^5%gms", timer.GetMilliSeconds());

    // don't free it's refrenced by the split packets.
    pPacket->pData = nullptr;
    pPacket->dataBitLength = 0;

    // we don't need this packt anymore.
    freePacket(pPacket);
    return true;
}

ReliablePacket* ReliabilityLayer::addIncomingSplitPacket(ReliablePacket* pPacket, core::TimeVal time)
{
    // we want to get the ordering channel for this split packet.
    auto splitId = pPacket->splitPacketId;

    auto lessVal = [](const SplitPacketChannel* pB, const SplitPacketId& splitId) -> bool {
        return pB->splitId < splitId;
    };
    auto greaterVal = [](const SplitPacketChannel* pB, const SplitPacketId& splitId) -> bool {
        return pB->splitId > splitId;
    };

    auto channelIt = splitPacketChannels_.findSortedKey(splitId, lessVal, greaterVal);

    SplitPacketChannel* pChannel = nullptr;

    if (channelIt == splitPacketChannels_.end()) {
        pChannel = allocSplicPacketChannel();
        pChannel->splitId = splitId;
        pChannel->packets.resize(pPacket->splitPacketCount); // we know how many we gonna get.

        splitPacketChannels_.insert_sorted(pChannel, [](const SplitPacketChannel* pA, const SplitPacketChannel* pB) -> bool {
            return pA->splitId < pB->splitId;
        });
    }
    else {
        pChannel = *channelIt;
    }

    X_ASSERT_NOT_NULL(pChannel);
    X_ASSERT(pChannel->splitId == splitId, "Got incorrect channel.")(splitId);
    X_ASSERT(pChannel->packets[pPacket->splitPacketIndex] == nullptr, "Index is already occupied.")(pPacket->splitPacketIndex);

    // meow.
    ++pChannel->packetsRecived;
    pChannel->lastUpdate = time;
    pChannel->packets[pPacket->splitPacketIndex] = pPacket;

    // are we complete? turing :D
    if (pChannel->haveAllPackets()) {
        // okay now we need to work out total size,
        auto& packets = pChannel->packets;

        size_t totalBitLength = core::accumulate(packets.begin(), packets.end(), 0_sz, [](const ReliablePacket* p) {
            return p->dataBitLength;
        });

        const ReliablePacket* pFirstPacket = packets[0];

        // allocate a buffer with new data?
        ReliablePacket* pRebuiltPacket = allocPacket();
        pRebuiltPacket->assignPropertiesExcData(pFirstPacket);

        // data..
        pRebuiltPacket->dataType = ReliablePacket::DataType::Normal;
        pRebuiltPacket->dataBitLength = safe_static_cast<BitSizeT>(totalBitLength);
        pRebuiltPacket->allocData(totalBitLength);

        // copy pasta!
        size_t dataBitOffset = 0;
        for (auto* p : packets) {
            std::memcpy(pRebuiltPacket->pData + core::bitUtil::bitsToBytes(dataBitOffset), p->pData, core::bitUtil::bitsToBytes(p->dataBitLength));
            dataBitOffset += p->dataBitLength;
        }

        freeSplitPacketChannel(pChannel);

        X_ASSERT(channelIt != splitPacketChannels_.end(), "Should not be merging split packs on frist packet")(channelIt);
        splitPacketChannels_.erase(channelIt);
        return pRebuiltPacket;
    }

    // report some sort of progress?
    {
    }

    return nullptr;
}

// -----------------------------------------

DataGramHistory* ReliabilityLayer::createDataGramHistory(DataGramSequenceNumber number, core::TimeVal time)
{
    if (dataGramHistory_.size() == REL_DATAGRAM_HISTORY_LENGTH) {
        dataGramHistory_.peek().messagenumbers.clear();
        dataGramHistory_.pop();
        ++dataGramHistoryPopCnt_;
    }

    dataGramHistory_.emplace(time);

    return &dataGramHistory_.back();
}

DataGramHistory* ReliabilityLayer::getDataGramHistory(DataGramSequenceNumber number)
{
    // so we have a sliding window for this.
    // in that we only have history for last sent datagram - REL_DATAGRAM_HISTORY_LENGTH
    // so if we recive a msg thats more than lastDataGram index - REL_DATAGRAM_HISTORY_LENGTH we have
    // to ignore the ack, and it will get resent.
    // these message indexs can also wrap around.

    if (number != dataGramHistoryPopCnt_) {
        // work out if lower that popcnt taking into account overflow.
        const DataGramSequenceNumber test = dataGramHistoryPopCnt_ - number;
        const DataGramSequenceNumber dataGramhalf = std::numeric_limits<DataGramSequenceNumber>::max() / 2;
        if (test < dataGramhalf) {
            return nullptr;
        }
    }

    DataGramSequenceNumber offset = number - dataGramHistoryPopCnt_;
    if (offset >= dataGramHistory_.size()) {
        return nullptr;
    }

    DataGramHistory* pHistory = &dataGramHistory_[offset];
#if X_DEBUG
    X_ASSERT(pHistory->magic == 0x12345678, "MAgic not match, corrupt item?")();
#endif // X_DEBUG
    return pHistory;
}

bool ReliabilityLayer::clearDataGramHistory(DataGramSequenceNumber number)
{
    DataGramSequenceNumber offset = number - dataGramHistoryPopCnt_;
    if (offset > dataGramHistory_.size()) {
        return false;
    }

    dataGramHistory_[offset].messagenumbers.clear();
    return true;
}

// -----------------------------------------

void ReliabilityLayer::insertPacketToResendList(ReliablePacket* pPacket)
{
    resendList_.insertTail(pPacket);
}

void ReliabilityLayer::movePacketToTailOfResendList(ReliablePacket* pPacket)
{
    pPacket->reliableLink.unlink();

    resendList_.insertTail(pPacket);
}

void ReliabilityLayer::removePacketFromResendList(MessageNumber msgNum)
{
    const auto resendBufIdx = resendBufferIdxForMsgNum(msgNum);

    ReliablePacket* pPacket = resendBuf_[resendBufIdx];
    if (pPacket && pPacket->reliableMessageNumber == msgNum) {
        resendBuf_[resendBufIdx] = nullptr;

        // stats
        --msgInReSendBuffers_;
        bytesInReSendBuffers_ -= core::bitUtil::bitsToBytes(pPacket->dataBitLength);
        // ~

        if (pPacket->isAckRequired() && (pPacket->splitPacketCount == 0 || pPacket->splitPacketIndex + 1 == pPacket->splitPacketCount)) {
            ReliablePacket* pAckPacket = allocPacket();
            pAckPacket->allocData(5 << 3);
            pAckPacket->pData[0] = MessageID::SndReceiptAcked;
            std::memcpy(&pAckPacket->pData[1], &pPacket->sendReceipt, sizeof(pPacket->sendReceipt));

            recivedPackets_.emplace(pAckPacket);
        }

        // now we need to remove.
        pPacket->reliableLink.unlink();

        // free it.
        freePacket(pPacket);
    }
    else {
        X_WARNING("NetRel", "Failed to packet from resend buffer for removal msgIdx: %" PRIu16 " bufIdx: %" PRIuS,
            msgNum, resendBufIdx);
    }
}

void ReliabilityLayer::freeResendList(void)
{
    while (ReliablePacket* pPacket = resendList_.head()) {
        freePacket(pPacket);
    }
}

// -----------------------------------------

size_t ReliabilityLayer::calculateMemoryUsage(void) const
{
    // meow meow.
    size_t size = 0;

    size += sizeof(*this);
    size += outGoingPackets_.capacity() * sizeof(decltype(outGoingPackets_)::Type);
    size += recivedPackets_.capacity() * sizeof(decltype(recivedPackets_)::Type);
    size += dataGramHistory_.capacity() * sizeof(decltype(dataGramHistory_)::Type);
    size += recivedPacketQueue_.capacity() * sizeof(decltype(recivedPacketQueue_)::Type);

    size += incomingAcks_.capacity() * sizeof(decltype(incomingAcks_)::RangeNodeType);
    size += naks_.capacity() * sizeof(decltype(naks_)::RangeNodeType);
    size += acks_.capacity() * sizeof(decltype(acks_)::RangeNodeType);

    for (const auto& q : orderingQueues_) {
        size += q.capacity() * sizeof(decltype(orderingQueues_)::value_type::Type);
    }

    for (const auto& bps : bps_) {
        size += bps.capacity() * sizeof(BPSTracker::Type);
    }

    size += splitPacketChannels_.capacity() * sizeof(decltype(splitPacketChannels_)::Type);
    size += splitPacketChannels_.size() * sizeof(SplitPacketChannel);

    return size;
}

void ReliabilityLayer::getStatistics(NetStatistics& stats)
{
    core::TimeVal time = gEnv->pTimer->GetTimeNowReal();

    for (uint32_t i = 0; i < NetStatistics::Metric::ENUM_COUNT; i++) {
        bps_[i].update(time);
    }

    static_cast<const ReliabilityLayer*>(this)->getStatistics(stats);
}

void ReliabilityLayer::getStatistics(NetStatistics& stats) const
{
    stats.internalMemUsage = calculateMemoryUsage();

    for (uint32_t i = 0; i < NetStatistics::Metric::ENUM_COUNT; i++) {
        stats.lastSecondMetrics[i] = bps_[i].getBPS();
        stats.runningMetrics[i] = bps_[i].getTotal();
    }

    stats.msgInSendBuffers = msgInSendBuffers_;
    stats.bytesInSendBuffers = bytesInSendBuffers_;

    if (stats.lastSecondMetrics[NetStatistics::Metric::BytesSent] || stats.lastSecondMetrics[NetStatistics::Metric::BytesResent]) {
        double sent = static_cast<double>(stats.lastSecondMetrics[NetStatistics::Metric::BytesSent]);
        double resent = static_cast<double>(stats.lastSecondMetrics[NetStatistics::Metric::BytesResent]);

        stats.packetLossLastSecond = static_cast<float>(resent / (sent + resent));
    }
    else {
        stats.packetLossLastSecond = 0.f;
    }

    if (stats.runningMetrics[NetStatistics::Metric::BytesSent] || stats.runningMetrics[NetStatistics::Metric::BytesResent]) {
        double sent = static_cast<double>(stats.runningMetrics[NetStatistics::Metric::BytesSent]);
        double resent = static_cast<double>(stats.runningMetrics[NetStatistics::Metric::BytesResent]);

        stats.packetLossTotal = static_cast<float>(resent / (sent + resent));
    }
    else {
        stats.packetLossTotal = 0.f;
    }

    stats.isLimitedByCongestionControl = false;
    stats.isLimitedByOutgoingBadwidthLimit = false;
}

size_t ReliabilityLayer::maxDataGramSize(void) const
{
    return MTUSize_;
}

size_t ReliabilityLayer::maxDataGramSizeExcHdr(void) const
{
    return maxDataGramSize() - sizeof(DatagramHdr);
}

size_t ReliabilityLayer::dataGramHdrSize(void) const
{
    return sizeof(DatagramHdr);
}

size_t ReliabilityLayer::dataGramHdrSizeBits(void) const
{
    return core::bitUtil::bytesToBits(dataGramHdrSize());
}

X_NAMESPACE_END