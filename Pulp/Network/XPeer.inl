
X_NAMESPACE_BEGIN(net)

X_INLINE bool PingAndClockDifferential::isValid(void) const
{
    return pingTime != UNDEFINED_PING;
}

// --------------------------------------------------------------

X_INLINE bool RemoteSystem::sendReliabile(const uint8_t* pData, BitSizeT numberOfBitsToSend, bool ownData, PacketPriority::Enum priority,
    PacketReliability::Enum reliability, uint8_t orderingChannel, core::TimeVal currentTime, SendReceipt receipt)
{
    if (!canSend()) {
        X_WARNING("Net", "Tried to send data to remote, where sending is currently disabled");
        return false;
    }

    bool res = relLayer.send(
        pData,
        numberOfBitsToSend,
        currentTime,
        MTUSize,
        priority,
        reliability,
        orderingChannel,
        receipt,
        ownData);

    onSend(reliability, currentTime);

    return res;
}

X_INLINE bool RemoteSystem::sendReliabile(const core::FixedBitStreamBase& bs, PacketPriority::Enum priority,
    PacketReliability::Enum reliability, uint8_t orderingChannel, core::TimeVal currentTime, SendReceipt receipt)
{
    if (!canSend()) {
        X_WARNING("Net", "Tried to send data to remote, where sending is currently disabled");
        return false;
    }

    bool res = relLayer.send(
        bs.data(),
        safe_static_cast<BitSizeT>(bs.size()),
        currentTime,
        MTUSize,
        priority,
        reliability,
        orderingChannel,
        receipt,
        false);

    onSend(reliability, currentTime);

    return res;
}

X_INLINE void RemoteSystem::onSend(PacketReliability::Enum reliability, core::TimeVal sendTime)
{
    switch (reliability) {
        case PacketReliability::Reliable:
        case PacketReliability::ReliableOrdered:
        case PacketReliability::ReliableOrderedWithAck:
        case PacketReliability::ReliableSequenced:
        case PacketReliability::ReliableWithAck:
            lastReliableSend = sendTime;
            break;
        default:
            break;
    }
}

// --------------------------------------------------------------

X_INLINE void XPeer::sendBuffered(const core::FixedBitStreamBase& bs, PacketPriority::Enum priority,
    PacketReliability::Enum reliability, uint8_t orderingChannel, SystemHandle systemHandle, bool broadcast, SendReceipt receipt)
{
    X_ASSERT(bs.isStartOfStream(), "Stream has been read from, potential bug?")();

    sendBuffered(
        bs.data(),
        safe_static_cast<BitSizeT>(bs.size()),
        priority,
        reliability,
        orderingChannel,
        systemHandle,
        broadcast,
        receipt);
}

X_INLINE bool XPeer::isLoopbackHandle(SystemHandle systemHandle)
{
    return systemHandle == LOOPBACK_SYSTEM_HANDLE;
}

X_INLINE void XPeer::pushBackPacket(Packet* pPacket)
{
    X_ASSERT_NOT_NULL(pPacket);
    packetQue_.push(pPacket);
}

X_INLINE void XPeer::pushPacket(MessageID::Enum msgId, const SystemAddressEx& sysAdd)
{
    pushPacket(msgId, sysAdd, UNASSIGNED_NET_GUID);
}

X_INLINE void XPeer::pushPacket(MessageID::Enum msgId, const SystemAddressEx& sysAdd, const NetGUID& guid)
{
    Packet* pPacket = allocPacket(core::bitUtil::bytesToBits(sizeof(MessageID::Enum) + sysAdd.serializedSize()));
    pPacket->systemHandle = INVALID_SYSTEM_HANDLE;
    pPacket->guid = guid;

    core::FixedBitStreamNoneOwning packetBs(pPacket->pData, pPacket->pData + pPacket->length, false);
    packetBs.write(msgId);
    sysAdd.writeToBitStream(packetBs);

    X_ASSERT(packetBs.freeSpace() == 0, "Unused spawce")(packetBs.size(), packetBs.sizeInBytes(), packetBs.capacity());

    packetQue_.push(pPacket); // fucking shove it right in!
}

X_INLINE void XPeer::pushPacket(MessageID::Enum msgId, const RemoteSystem& rs)
{
    Packet* pPacket = allocPacket(core::bitUtil::bytesToBits(sizeof(MessageID::Enum) + rs.systemAddress.serializedSize()));
    pPacket->systemHandle = rs.getHandle();
    pPacket->guid = rs.guid;

    core::FixedBitStreamNoneOwning packetBs(pPacket->pData, pPacket->pData + pPacket->length, false);
    packetBs.write(msgId);
    rs.systemAddress.writeToBitStream(packetBs);

    X_ASSERT(packetBs.freeSpace() == 0, "Unused spawce")(packetBs.size(), packetBs.sizeInBytes(), packetBs.capacity());

    packetQue_.push(pPacket); // right in the pickle!
}

X_INLINE SendReceipt XPeer::nextSendReceipt(void)
{
    return sendReceiptSerial_;
}

X_INLINE SendReceipt XPeer::incrementNextSendReceipt(void)
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
