#pragma once

#include <Containers\Fifo.h>

X_NAMESPACE_BEGIN(net)

class NetSocket;
class NetVars;

typedef uint16_t SplitPacketId;
typedef uint32_t SplitPacketIndex;
typedef uint16_t MessageNumber;
typedef uint16_t OrderingIndex;


struct PacketHeader
{
	MessageNumber reliableMessageNumber;
	OrderingIndex orderingIndex;
	OrderingIndex sequencingIndex;

	uint8_t orderingChannel;
	SplitPacketId splitPacketId;
	SplitPacketIndex splitPacketIndex;
	SplitPacketIndex splitPacketCount;;
	BitSizeT dataBitLength;
	PacketReliability::Enum reliability;
};

struct RelPacket : public PacketHeader
{

	core::TimeVal creationTime;			// 
	core::TimeVal nextActionTime;		// 
	core::TimeVal retransmissionTime;	// 

	uint8_t sendAttemps;
	PacketPriority::Enum priority;
};

static_assert(core::compileTime::IsPOD<PacketHeader>::Value, "Packet header should be POD");

class ReliabilityLayer
{
	typedef std::array<OrderingIndex, MAX_ORDERED_STREAMS> OrdereIndexArr;
	typedef core::Fifo<RelPacket*> PacketQeue;

public:
	ReliabilityLayer(NetVars& vars, core::MemoryArenaBase* arena, core::MemoryArenaBase* packetPool);
	~ReliabilityLayer();



	bool send(const uint8_t* pData, const BitSizeT lengthBits, core::TimeVal time, uint32_t mtuSize,
		PacketPriority::Enum priority, PacketReliability::Enum reliability, uint8_t orderingChannel,  uint32_t receipt);


	void update(NetSocket& socket, SystemAdd& systemAddress, int32_t MTUSize, 
		core::TimeVal time, size_t bitsPerSecondLimit);


	X_INLINE void setTimeout(core::TimeVal timeout);
	X_INLINE core::TimeVal getTimeout(void);

	X_INLINE void setUnreliableMsgTimeout(core::TimeVal timeout);
	X_INLINE core::TimeVal getUnreliableMsgTimeout(void);

private:
	RelPacket* allocPacket(void);


private:
	NetVars& vars_;

	core::MemoryArenaBase* packetPool_;

	core::TimeVal timeOut_;
	core::TimeVal unreliableTimeOut_;


	OrdereIndexArr orderedWriteIndex_;				// inc for every ordered msg sent
	OrdereIndexArr sequencedWriteIndex_;			// inc for every sequenced msg sent
	OrdereIndexArr orderedReadIndex_;				// next 'expected' ordered index
	OrdereIndexArr highestSequencedReadIndex_;		// higest value received for sequencedWriteIndex


	PacketQeue outGoingPackets_;
};

X_NAMESPACE_END

#include "ReliabilityLayer.inl"