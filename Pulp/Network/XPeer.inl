
X_NAMESPACE_BEGIN(net)

X_INLINE bool PingAndClockDifferential::isValid(void) const
{
	return pingTime != UNDEFINED_PING;
}

X_INLINE void XPeer::sendBuffered(const core::FixedBitStreamBase& bs, PacketPriority::Enum priority,
	PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t receipt)
{
	X_ASSERT(bs.isStartOfStream(), "Stream has been read from, potential bug?")();

	sendBuffered(
		bs.data(),
		safe_static_cast<BitSizeT>(bs.size()),
		priority,
		reliability,
		orderingChannel,
		systemIdentifier,
		broadcast,
		receipt
	);
}


X_INLINE bool XPeer::sendImmediate(const core::FixedBitStreamBase& bs, PacketPriority::Enum priority,
	PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast,
	core::TimeVal currentTime, uint32_t receipt)
{
	// try to catch potential bugs, where stream that is to be send has been read from.
	X_ASSERT(bs.isStartOfStream(), "Stream has been read from, potential bug?")();

	return sendImmediate(
		bs.data(),
		safe_static_cast<BitSizeT>(bs.size()),
		priority,
		reliability,
		orderingChannel,
		systemIdentifier,
		broadcast,
		currentTime,
		receipt
	);
}

X_INLINE void XPeer::pushBackPacket(Packet* pPacket)
{
	X_ASSERT_NOT_NULL(pPacket);

	packetQue_.push(pPacket);
}

X_INLINE uint32_t XPeer::nextSendReceipt(void)
{
	return sendReceiptSerial_;
}

X_INLINE uint32_t XPeer::incrementNextSendReceipt(void)
{
	return ++sendReceiptSerial_;
}

X_INLINE uint16_t XPeer::getMaximumIncomingConnections(void) const
{
	return maxIncommingConnections_;
}

X_INLINE uint32_t XPeer::getMaximunNumberOfPeers(void) const
{
	return maxPeers_;
}


X_INLINE const NetGUID& XPeer::getMyGUID(void) const
{
	return guid_;
}


X_INLINE void XPeer::setUnreliableTimeout(core::TimeVal timeout)
{
	unreliableTimeOut_ = timeout;
}

X_INLINE bool XPeer::accpetingIncomingConnections(void) const
{
	return getNumRemoteInitiatedConnections() < getMaximumIncomingConnections();
}

X_INLINE void XPeer::onSocketRecv(RecvData* pData)
{
	// we own this pointer
	X_ASSERT_NOT_NULL(pData);

	recvDataQue_.push(pData);
}



X_NAMESPACE_END
