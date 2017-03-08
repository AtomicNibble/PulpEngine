#pragma once

#include "Util\BPSTracker.h"
#include "Util\RangeList.h"

#include <Containers\Fifo.h>

X_NAMESPACE_BEGIN(net)

class NetSocket;
class NetVars;

typedef uint16_t SplitPacketId;
typedef uint32_t SplitPacketIndex;
typedef uint16_t MessageNumber;
typedef uint16_t OrderingIndex;
typedef uint16_t DataGramSequenceNumber;


struct ReliablePacket
{
	ReliablePacket();

	bool isReliable(void) const;
	size_t getHeaderLengthBits(void) const; // returns number of bits needed to store this header. it's diffrent depending on priority types etc.

	void writeToBitStream(core::FixedBitStreamBase& bs) const;
	bool fromBitStream(core::FixedBitStreamBase& bs);

public:
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
	core::TimeVal retransmissionTime;	// 
	core::TimeVal nextActionTime;		// 

	BitSizeT dataBitLength;
	uint8_t* pData;
};

static const size_t goat = sizeof(ReliablePacket);
// static_assert(core::compileTime::IsPOD<RelPacket>::Value, "Packet header should be POD");

struct DataGramHistory
{
	typedef core::FixedArray<MessageNumber, MAX_PACKETS_PER_DATAGRAM> MesgNumberArr;

public:
	DataGramHistory(core::TimeVal timesent) : timeSent(timesent)
	{}

	core::TimeVal timeSent;
	MesgNumberArr messagenumbers;
};


class ReliabilityLayer
{

	typedef std::array<OrderingIndex, MAX_ORDERED_STREAMS> OrdereIndexArr;
	typedef std::array<ReliablePacket*, REL_RESEND_BUF_LENGTH> ResendArr;
	typedef core::Fifo<DataGramHistory> DataGramHistoryQeue;
	typedef core::Fifo<ReliablePacket*> PacketQeue;

public:
	struct PacketData
	{
		uint8_t* pData;
		BitSizeT numBits;
	};

	typedef RangeList<DataGramSequenceNumber> DataGramNumberRangeList;

	X_NO_COPY(ReliabilityLayer);
	X_NO_ASSIGN(ReliabilityLayer);

public:
	ReliabilityLayer(NetVars& vars, core::MemoryArenaBase* arena, core::MemoryArenaBase* packetPool);
	ReliabilityLayer(ReliabilityLayer&& oth) = default;
	~ReliabilityLayer();

	ReliabilityLayer& operator=(ReliabilityLayer&& rhs) = default;

	void reset(int32_t MTUSize);

	// que some data for sending, reliability is handled.
	bool send(const uint8_t* pData, const BitSizeT lengthBits, core::TimeVal time, uint32_t mtuSize,
		PacketPriority::Enum priority, PacketReliability::Enum reliability, uint8_t orderingChannel,  uint32_t receipt);

	// pass data from socket for processing
	bool recv(uint8_t* pData, const size_t lengt, NetSocket& socket,
		SystemAdd& systemAddress, core::TimeVal time, uint32_t mtuSize);

	// update internal logic, re-send packets / other reliability actions.
	void update(core::FixedBitStreamBase& bs, NetSocket& socket, SystemAdd& systemAddress, int32_t MTUSize,
		core::TimeVal time);

	// pop any packets that have arrived.
	bool recive(PacketData& dataOut);


	void getStatistics(NetStatistics& stats) const;

	X_INLINE bool pendingOutgoingData(void) const;
	X_INLINE bool isWaitingForAcks(void) const;
	X_INLINE bool isConnectionDead(void) const;
	X_INLINE void killConnection(void);


private:
	X_INLINE void addAck(DataGramSequenceNumber messageNumber);
	size_t maxDataGramSize(void) const;
	size_t maxDataGramSizeExcHdr(void) const;

	X_INLINE BitSizeT maxDataGramSizeBits(void) const;
	X_INLINE BitSizeT maxDataGramSizeExcHdrBits(void) const;

private:
	bool hasTimedOut(core::TimeVal time);

	void sendACKs(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAdd& systemAddress, core::TimeVal time);
	void sendNAKs(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAdd& systemAddress, core::TimeVal time);
	void sendBitStream(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAdd& systemAddress, core::TimeVal time);

	ReliablePacket* packetFromBS(core::FixedBitStreamBase& bs, core::TimeVal time);
	ReliablePacket* allocPacket(void);
	void freePacket(ReliablePacket* pPacker);

private:
	// these are created each time a data gram is sent.
	// so if a packet gets resent a new one will be sent.
	DataGramHistory* createDataGramHistory(DataGramSequenceNumber number, core::TimeVal time);
	DataGramHistory* getDataGramHistory(DataGramSequenceNumber number);
	bool clearDataGramHistory(DataGramSequenceNumber number);

private:
	NetVars& vars_;

	core::MemoryArenaBase* packetPool_;

	int32_t MTUSize_;

	core::TimeVal timeLastDatagramArrived_;
	core::TimeVal lastBSPUpdate_;


	OrdereIndexArr orderedWriteIndex_;				// inc for every ordered msg sent
	OrdereIndexArr sequencedWriteIndex_;			// inc for every sequenced msg sent
	OrdereIndexArr orderedReadIndex_;				// next 'expected' ordered index
	OrdereIndexArr highestSequencedReadIndex_;		// higest value received for sequencedWriteIndex

	PacketQeue outGoingPackets_;
	PacketQeue recivedPackets_;
	DataGramHistoryQeue dataGramHistory_;
	DataGramSequenceNumber dataGramHistoryPopCnt_;

	DataGramNumberRangeList incomingAcks_;
	DataGramNumberRangeList naks_;
	DataGramNumberRangeList acks_;

	MessageNumber reliableMessageNumberIdx_; // current rel msg number index.
	MessageNumber dagramSeqNumber_; 


	bool connectionDead_;
	bool _pad[3];

	BPSTracker bps_[NetStatistics::Metric::ENUM_COUNT];

	NetStatistics::PriorityMsgCountsArr msgInSendBuffers_;
	NetStatistics::PriorityByteCountsArr bytesInSendBuffers_;
	size_t bytesInReSendBuffers_;
	size_t msgInReSendBuffers_;

	ResendArr resendBuf_;
};

X_NAMESPACE_END

#include "ReliabilityLayer.inl"