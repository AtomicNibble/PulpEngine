#pragma once

#include <Containers\Fifo.h>

X_NAMESPACE_BEGIN(net)

class NetSocket;
class NetVars;

typedef uint16_t SplitPacketId;
typedef uint32_t SplitPacketIndex;
typedef uint16_t MessageNumber;
typedef uint16_t OrderingIndex;

typedef core::FixedBitStream<core::FixedBitStreamNoneOwningPolicy> FixedBitStream;


struct ReliablePacket
{
	ReliablePacket();

	void writeToBitStream(FixedBitStream& bs) const;
	bool fromBitStream(FixedBitStream& bs);

	MessageNumber reliableMessageNumber;
	OrderingIndex orderingIndex;
	OrderingIndex sequencingIndex;

	PacketReliability::Enum reliability;
	PacketPriority::Enum priority;
	uint8_t sendAttemps;

	uint8_t orderingChannel;
	SplitPacketId splitPacketId;
	SplitPacketIndex splitPacketIndex;
	SplitPacketIndex splitPacketCount;

	core::TimeVal creationTime;			// 
	core::TimeVal nextActionTime;		// 
	core::TimeVal retransmissionTime;	// 

	BitSizeT dataBitLength;
	uint8_t* pData;
};

static const size_t goat = sizeof(ReliablePacket);
// static_assert(core::compileTime::IsPOD<RelPacket>::Value, "Packet header should be POD");


class ReliabilityLayer
{
	typedef std::array<OrderingIndex, MAX_ORDERED_STREAMS> OrdereIndexArr;
	typedef core::Fifo<ReliablePacket*> PacketQeue;

public:
	struct PacketData
	{
		uint8_t* pData;
		BitSizeT numBits;
	};

public:
	ReliabilityLayer(NetVars& vars, core::MemoryArenaBase* arena, core::MemoryArenaBase* packetPool);
	~ReliabilityLayer();

	// que some data for sending, reliability is handled.
	bool send(const uint8_t* pData, const BitSizeT lengthBits, core::TimeVal time, uint32_t mtuSize,
		PacketPriority::Enum priority, PacketReliability::Enum reliability, uint8_t orderingChannel,  uint32_t receipt);

	// pass data from socket for processing
	bool recv(uint8_t* pData, const size_t lengt, NetSocket& socket,
		SystemAdd& systemAddress, core::TimeVal time, uint32_t mtuSize);

	// update internal logic, re-send packets / other reliability actions.
	void update(FixedBitStream& bs, NetSocket& socket, SystemAdd& systemAddress, int32_t MTUSize,
		core::TimeVal time, size_t bitsPerSecondLimit);

	// pop any packets that have arrived.
	bool recive(PacketData& dataOut);


	X_INLINE void setTimeout(core::TimeVal timeout);
	X_INLINE core::TimeVal getTimeout(void);

	X_INLINE void setUnreliableMsgTimeout(core::TimeVal timeout);
	X_INLINE core::TimeVal getUnreliableMsgTimeout(void);

private:
	void sendBitStream(NetSocket& socket, FixedBitStream& bs, SystemAdd& systemAddress);

	ReliablePacket* allocPacket(void);
	void freePacket(ReliablePacket* pPacker);


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
	PacketQeue recivedPackets_;
};

X_NAMESPACE_END

#include "ReliabilityLayer.inl"