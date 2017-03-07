

X_NAMESPACE_BEGIN(net)



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


X_NAMESPACE_END