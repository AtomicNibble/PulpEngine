

X_NAMESPACE_BEGIN(net)

X_INLINE uint16_t NetVars::port(void) const
{
    return safe_static_cast<uint16_t>(port_);
}

X_INLINE int32_t NetVars::debugEnabled(void) const
{
    return debug_;
}

X_INLINE int32_t NetVars::debugIgnoredEnabled(void) const
{
    return debugIgnored_;
}

X_INLINE int32_t NetVars::debugDatagramEnabled(void) const
{
    return debugDataGram_;
}

X_INLINE int32_t NetVars::debugAckEnabled(void) const
{
    return debugAck_;
}

X_INLINE int32_t NetVars::debugNackEnabled(void) const
{
    return debugNACk_;
}

X_INLINE int32_t NetVars::debugSocketsEnabled(void) const
{
    return debugSockets_;
}

X_INLINE bool NetVars::rlConnectionsPerIP(void) const
{
    return rlconnectionsPerIp_ != 0;
}

X_INLINE int32_t NetVars::rlConnectionsPerIPThreshMS(void) const
{
    return rlconnectionsPerIpThreshMS_;
}

X_INLINE int32_t NetVars::rlConnectionsPerIPBanTimeMS(void) const
{
    return rlconnectionsPerIpBanTimeMS_;
}

X_INLINE int32_t NetVars::defaultTimeoutMS(void) const
{
    return defaultTimeoutMS_;
}

X_INLINE int32_t NetVars::dropPartialConnectionsMS(void) const
{
    return dropPartialConnectionsMS_;
}

X_INLINE int32_t NetVars::pingTimeMS(void) const
{
    return pingTimeMS_;
}

X_INLINE int32_t NetVars::unexpectedMsgBanTime(void) const
{
    return unexpectedMsgBanTime_;
}

X_INLINE int32_t NetVars::connectionBSPLimit(void) const
{
    return connectionBSPLimit_;
}

X_INLINE bool NetVars::artificalNetworkEnabled(void) const
{
    return artificalNetwork_ != 0;
}

X_INLINE float32_t NetVars::artificalPacketLoss(void) const
{
    return artificalPacketLoss_;
}

X_INLINE int32_t NetVars::artificalPing(void) const
{
    return artificalPing_;
}

X_INLINE int32_t NetVars::artificalPingVariance(void) const
{
    return artificalPingVariance_;
}

X_INLINE bool NetVars::ignorePasswordFromClientIfNotRequired(void) const
{
    return true;
}

X_INLINE core::Compression::Algo::Enum NetVars::ackCompAlgo(void) const
{
    X_ASSERT(ackCompAlgo_ >= 0 && ackCompAlgo_ < core::Compression::Algo::ENUM_COUNT, "Algo enum out of range")();
    return static_cast<core::Compression::Algo::Enum>(ackCompAlgo_);
}

X_INLINE core::Compression::Algo::Enum NetVars::compAlgo(void) const
{
    X_ASSERT(ackCompAlgo_ >= 0 && ackCompAlgo_ < core::Compression::Algo::ENUM_COUNT, "Algo enum out of range")();
    return static_cast<core::Compression::Algo::Enum>(compAlgo_);
}

X_NAMESPACE_END