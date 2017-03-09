
X_NAMESPACE_BEGIN(net)

X_INLINE bool PingAndClockDifferential::isValid(void) const
{
	return pingTime != UNDEFINED_PING;
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



X_NAMESPACE_END
