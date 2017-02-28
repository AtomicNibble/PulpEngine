#include "stdafx.h"
#include "ReliabilityLayer.h"

#include "Vars\NetVars.h"

X_NAMESPACE_BEGIN(net)


ReliabilityLayer::ReliabilityLayer(NetVars& vars, core::MemoryArenaBase* arena, core::MemoryArenaBase* packetPool) :
	vars_(vars),
	packetPool_(packetPool),
	outGoingPackets_(arena)
{
	outGoingPackets_.reserve(128);
}

ReliabilityLayer::~ReliabilityLayer()
{

}



bool ReliabilityLayer::send(const uint8_t* pData, const BitSizeT lengthBits, core::TimeVal time, uint32_t mtuSize,
	PacketPriority::Enum priority, PacketReliability::Enum reliability, uint8_t orderingChannel, uint32_t receipt)
{
	X_ASSERT_NOT_NULL(pData);
	X_ASSERT(lengthBits > 0, "Must call with alreast some bits")();


	X_LOG0_IF(vars_.debugEnabled(), "Net", "Reliable msg added");
	
	RelPacket* pPacket = allocPacket();
	pPacket->creationTime = time;
	pPacket->dataBitLength = lengthBits;
	pPacket->priority = priority;
	pPacket->reliability = reliability;
	pPacket->sendAttemps = 0;

	// sequenced packets
	if (reliability == PacketReliability::ReliableSequenced || reliability == PacketReliability::UnReliableSequenced)
	{
		pPacket->orderingChannel = orderingChannel;
		pPacket->orderingIndex = orderedWriteIndex_[orderingChannel];
		pPacket->sequencingIndex = sequencedWriteIndex_[orderingChannel]++;

	}
	else if (reliability == PacketReliability::ReliableOrdered || reliability == PacketReliability::ReliableOrderedWithAck)
	{
		pPacket->orderingChannel = orderingChannel;
		pPacket->orderingIndex = orderedWriteIndex_[orderingChannel]++;

		sequencedWriteIndex_[orderingChannel] = 0;
	}
	else
	{
		pPacket->orderingChannel = std::numeric_limits<decltype(pPacket->orderingChannel)>::max();
		pPacket->orderingIndex = std::numeric_limits<OrderingIndex>::max();
		pPacket->sequencingIndex = std::numeric_limits<OrderingIndex>::max();
	
	}
	
	outGoingPackets_.push(pPacket);
	return true;
}

void ReliabilityLayer::update(NetSocket& socket, SystemAdd& systemAddress, int32_t MTUSize, 
	core::TimeVal time, size_t bitsPerSecondLimit)
{


}

RelPacket* ReliabilityLayer::allocPacket(void)
{
	return X_NEW(RelPacket, packetPool_, "RelPacket");
}

X_NAMESPACE_END