

X_NAMESPACE_BEGIN(net)


X_INLINE ReliabilityLayer::PacketData::PacketData() :
	numBits_(0),
	pData_(nullptr),
	arena_(nullptr)
{
}

X_INLINE ReliabilityLayer::PacketData::~PacketData()
{
	if (pData_) {
		X_DELETE_ARRAY(pData_, arena_);
	}
}

X_INLINE void ReliabilityLayer::PacketData::setdata(uint8_t* pData, BitSizeT numBits, core::MemoryArenaBase* arena) 
{
	if (pData_) {
		X_DELETE_ARRAY(pData_, arena_);
	}

	numBits_ = numBits;
	pData_ = pData;
	arena_ = arena;
}

X_INLINE BitSizeT ReliabilityLayer::PacketData::getNumbBits(void) const 
{
	return numBits_;
}

X_INLINE uint8_t* ReliabilityLayer::PacketData::getData(void) const 
{
	return pData_;
}

X_INLINE uint8_t* ReliabilityLayer::PacketData::begin(void) 
{
	return pData_;
}

X_INLINE uint8_t* ReliabilityLayer::PacketData::end(void) 
{
	return pData_ + core::bitUtil::bitsToBytes(numBits_);
}

X_INLINE const uint8_t* ReliabilityLayer::PacketData::begin(void) const
{
	return pData_;
}

X_INLINE const uint8_t* ReliabilityLayer::PacketData::end(void) const
{
	return pData_ + core::bitUtil::bitsToBytes(numBits_);
}

// --------------------------------------------------

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
	return msgNum % resendBuf_.max_size();
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

X_NAMESPACE_END