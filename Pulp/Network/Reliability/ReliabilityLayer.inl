

X_NAMESPACE_BEGIN(net)

X_INLINE PacketData::PacketData(core::MemoryArenaBase* arena) :
    numBits_(0),
    data_(arena)
{
}

X_INLINE PacketData::~PacketData()
{
}

X_INLINE void PacketData::setdata(uint8_t* pData, BitSizeT numBits, core::MemoryArenaBase* arena)
{
    numBits_ = numBits;
    data_ = core::UniquePointer<uint8_t[]>(arena, pData);
}

X_INLINE core::UniquePointer<uint8_t[]>& PacketData::getUP(void)
{
    return data_;
}

X_INLINE BitSizeT PacketData::getNumbBits(void) const
{
    return numBits_;
}

X_INLINE uint8_t* PacketData::getData(void) const
{
    return data_.ptr();
}

X_INLINE uint8_t* PacketData::begin(void)
{
    return data_.ptr();
}

X_INLINE uint8_t* PacketData::end(void)
{
    return data_.ptr() + core::bitUtil::bitsToBytes(numBits_);
}

X_INLINE const uint8_t* PacketData::begin(void) const
{
    return data_.ptr();
}

X_INLINE const uint8_t* PacketData::end(void) const
{
    return data_.ptr() + core::bitUtil::bitsToBytes(numBits_);
}

// --------------------------------------------------

X_INLINE size_t ReliabilityLayer::numMsgInResendBuffer(void) const
{
    // could I just use 'resendList_.isEmpty()' 
    if (msgInReSendBuffers_ == 0) {
        X_ASSERT(resendList_.isEmpty(), "Resent list should be empty")(resendList_.isEmpty());
    }

    return msgInReSendBuffers_;
}

X_INLINE bool ReliabilityLayer::pendingOutgoingData(void) const
{
    return outGoingPackets_.isNotEmpty();
}


X_INLINE bool ReliabilityLayer::isWaitingForAcks(void) const
{
    return acks_.isNotEmpty();
}

X_INLINE bool ReliabilityLayer::isConnectionDead(void) const
{
    return connectionDead_;
}

X_INLINE void ReliabilityLayer::killConnection(void)
{
    connectionDead_ = true;
}

X_INLINE void ReliabilityLayer::addAck(DataGramSequenceNumber messageNumber)
{
    acks_.add(messageNumber);
}

X_INLINE BitSizeT ReliabilityLayer::maxDataGramSizeBits(void) const
{
    return safe_static_cast<BitSizeT>(core::bitUtil::bytesToBits(maxDataGramSize()));
}

X_INLINE BitSizeT ReliabilityLayer::maxDataGramSizeExcHdrBits(void) const
{
    return safe_static_cast<BitSizeT>(core::bitUtil::bytesToBits(maxDataGramSizeExcHdr()));
}

X_INLINE size_t ReliabilityLayer::resendBufferIdxForMsgNum(MessageNumber msgNum) const
{
    return msgNum % resendBuf_.size();
}

X_INLINE bool ReliabilityLayer::isResendBufferFull(void) const
{
    const auto curIdx = resendBufferIdxForMsgNum(reliableMessageNumberIdx_);

    return resendBuf_[curIdx] != nullptr;
}

X_INLINE bool ReliabilityLayer::isResendListEmpty(void) const
{
    return resendList_.isEmpty();
}

X_INLINE bool ReliabilityLayer::isOlderPacket(OrderingIndex packetIdx, OrderingIndex currentIdx)
{
    //   const OrderingIndex rangeMax = std::numeric_limits<OrderingIndex>::max();
    const OrderingIndex rangeHalf = std::numeric_limits<OrderingIndex>::max() / 2;

    // takes into account wrap around.
    // so once current is above hald type range.
    // packetIdx must be higer than current in order to be older.

    if (currentIdx > rangeHalf) {
        const OrderingIndex shiftedCurrent = currentIdx - (rangeHalf + 1);

        if (packetIdx < currentIdx && packetIdx >= shiftedCurrent) {
            return true;
        }
    }
    else {
        const OrderingIndex shiftedCurrent = currentIdx - (rangeHalf + 1);

        if (packetIdx < currentIdx || packetIdx >= shiftedCurrent) {
            return true;
        }
    }

    return false;
}

X_NAMESPACE_END