#include "stdafx.h"

#include <Memory\AllocationPolicies\StackAllocator.h>
#include <ITimer.h>

#include "ReliabilityLayer.h"

#include "Sockets\Socket.h"
#include "Vars\NetVars.h"


X_NAMESPACE_BEGIN(net)

namespace
{
	// move to bit util maybe :)

	X_DECLARE_FLAGS(DatagramFlag) (
		Ack,
		Nack
	);

	typedef Flags<DatagramFlag> DatagramFlags;

	X_PACK_PUSH(1)
	struct DatagramHdr
	{
		void writeToBitStream(core::FixedBitStreamBase& bs) const;
		void fromBitStream(core::FixedBitStreamBase& bs);

		uint16_t number;
		DatagramFlags flags;
	};
	X_PACK_POP;

	void DatagramHdr::writeToBitStream(core::FixedBitStreamBase& bs) const
	{
		bs.write(number);
		bs.write(flags);
		bs.alignWriteToByteBoundry();
	}

	void DatagramHdr::fromBitStream(core::FixedBitStreamBase& bs)
	{
		bs.read(number);
		bs.read(flags);
		bs.alignReadToByteBoundry();
	}

} // namespace

ReliablePacket::ReliablePacket()
{
	reliableMessageNumber = std::numeric_limits<decltype(reliableMessageNumber)>::max();
	orderingIndex = 0;
	sequencingIndex = 0;

	orderingChannel = 0;
	splitPacketId = 0;
	splitPacketIndex = 0;
	splitPacketCount = 0;
	dataBitLength = 0;
	pData = nullptr;
}


void ReliablePacket::writeToBitStream(core::FixedBitStreamBase& bs) const
{

	bs.writeBits(reinterpret_cast<const uint8_t*>(&reliability), core::bitUtil::bitsNeededForValue(PacketReliability::ENUM_COUNT));
	bs.write<uint16_t>(safe_static_cast<uint16_t>(dataBitLength)); 

	// reliable types.
	if (reliability == PacketReliability::Reliable ||
		reliability == PacketReliability::ReliableOrdered ||
		reliability == PacketReliability::ReliableSequenced ||
		reliability == PacketReliability::ReliableOrderedWithAck ||
		reliability == PacketReliability::ReliableWithAck)
	{
		bs.write(reliableMessageNumber);
	}

	// sequenced.
	if (reliability == PacketReliability::UnReliableSequenced ||
		reliability == PacketReliability::ReliableSequenced)
	{
		bs.write(sequencingIndex);
	}

	// ordered
	if (reliability == PacketReliability::UnReliableSequenced ||
		reliability == PacketReliability::ReliableSequenced ||
		reliability == PacketReliability::ReliableOrdered ||
		reliability == PacketReliability::ReliableOrderedWithAck)
	{
		bs.write(orderingIndex);
		bs.write(orderingChannel);
	}

	bs.writeAligned(pData, core::bitUtil::bitsToBytes(dataBitLength));
}


bool ReliablePacket::fromBitStream(core::FixedBitStreamBase& bs)
{
	uint8_t rel;
	uint16_t bits;
	bs.readBits(&rel, core::bitUtil::bitsNeededForValue(PacketReliability::ENUM_COUNT));
	bs.read(bits);

	reliability = static_cast<PacketReliability::Enum>(rel);
	dataBitLength = bits;

	// reliable types.
	if (reliability == PacketReliability::Reliable ||
		reliability == PacketReliability::ReliableOrdered ||
		reliability == PacketReliability::ReliableSequenced ||
		reliability == PacketReliability::ReliableOrderedWithAck ||
		reliability == PacketReliability::ReliableWithAck)
	{
		bs.read(reliableMessageNumber);
	}

	// sequenced.
	if (reliability == PacketReliability::UnReliableSequenced ||
		reliability == PacketReliability::ReliableSequenced)
	{
		bs.read(sequencingIndex);
	}

	// ordered
	if (reliability == PacketReliability::UnReliableSequenced ||
		reliability == PacketReliability::ReliableSequenced ||
		reliability == PacketReliability::ReliableOrdered ||
		reliability == PacketReliability::ReliableOrderedWithAck)
	{
		bs.read(orderingIndex);
		bs.read(orderingChannel);
	}

	pData = X_NEW_ARRAY(uint8_t, core::bitUtil::bitsToBytes(dataBitLength), g_NetworkArena, "PacketBytes");

	bs.readBitsAligned(pData, dataBitLength);
	return true;
}

// ---------------------------

ReliabilityLayer::ReliabilityLayer(NetVars& vars, core::MemoryArenaBase* arena, core::MemoryArenaBase* packetPool) :
	vars_(vars),
	MTUSize_(0),
	packetPool_(packetPool),
	outGoingPackets_(arena),
	recivedPackets_(arena),
	connectionDead_(false),
	incomingAcks_(arena),
	naks_(arena),
	acks_(arena),
	bps_{ arena, arena, arena, arena, arena, arena, arena }
{
	outGoingPackets_.reserve(128);
	recivedPackets_.reserve(128);
}

ReliabilityLayer::~ReliabilityLayer()
{

}


void ReliabilityLayer::reset(int32_t MTUSize)
{
	MTUSize_ = MTUSize;

	timeLastDatagramArrived_ = gEnv->pTimer->GetTimeNowReal();

	connectionDead_ = false;

	msgInSendBuffers_.fill(0);
	bytesInSendBuffers_.fill(0);

	for (uint32_t i = 0; i < NetStatistics::Metric::ENUM_COUNT; i++) {
		bps_[i].reset();
	}
}


bool ReliabilityLayer::send(const uint8_t* pData, const BitSizeT lengthBits, core::TimeVal time, uint32_t mtuSize,
	PacketPriority::Enum priority, PacketReliability::Enum reliability, uint8_t orderingChannel, uint32_t receipt)
{
	X_ASSERT_NOT_NULL(pData);
	X_ASSERT(lengthBits > 0, "Must call with alreast some bits")();

	X_LOG0_IF(vars_.debugEnabled(), "NetRel", "'%s' msg added. Pri: \"%s\" numbits: %" PRIu32, 
		PacketReliability::ToString(reliability), PacketPriority::ToString(priority), lengthBits);
	
	auto lengthBytes = core::bitUtil::bitsToBytes(lengthBits);

	ReliablePacket* pPacket = allocPacket();
	pPacket->reliableMessageNumber = 0;
	pPacket->creationTime = time;
	pPacket->dataBitLength = lengthBits;
	pPacket->priority = priority;
	pPacket->reliability = reliability;
	pPacket->sendAttemps = 0;

	// split me open.
	pPacket->splitPacketId = 0;
	pPacket->splitPacketIndex = 0;
	pPacket->splitPacketCount = 0;
	
	pPacket->dataBitLength = lengthBits;
	pPacket->pData = X_NEW_ARRAY(uint8_t, lengthBytes, g_NetworkArena, "PacketBytes");

	std::memcpy(pPacket->pData, pData, lengthBytes);
	// pPacket->pData = const_cast<uint8_t*>(pData);

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

	bps_[NetStatistics::Metric::BytesPushed].add(time, lengthBytes);

	++msgInSendBuffers_[priority];
	bytesInSendBuffers_[priority] += lengthBytes;
	return true;
}

bool ReliabilityLayer::recv(uint8_t* pData, const size_t length, NetSocket& socket,
	SystemAdd& systemAddress, core::TimeVal time, uint32_t mtuSize)
{
	X_LOG0_IF(vars_.debugEnabled(), "NetRel", "Recived reliable packet numbits: %" PRIuS, core::bitUtil::bytesToBits(length));

	// last time we got data.
	timeLastDatagramArrived_ = gEnv->pTimer->GetTimeNowReal();

	bps_[NetStatistics::Metric::ActualBytesReceived].add(time, length);

	core::FixedBitStreamNoneOwning bs(pData, pData + length, true);

	DatagramHdr dgh;
	dgh.fromBitStream(bs);

	if (vars_.debugEnabled())
	{
		DatagramFlags::Description Dsc;
		X_LOG0("NetRel", "DataGram number: %" PRIu16 "Flags: \"%s\"", dgh.number, dgh.flags.ToString(Dsc));
	}

	if (dgh.flags.IsSet(DatagramFlag::Ack))
	{
		// ack, ack, ack!
		incomingAcks_.clear();

		if (!incomingAcks_.fromBitStream(bs)) {
			X_ERROR("NetRel", "Failed to process incomming ack's");
			return false;
		}

		for (auto& ackRange : incomingAcks_)
		{
			// we want to mark all these messages as recived so we don't resend like a pleb.


		}

	}
	else if (dgh.flags.IsSet(DatagramFlag::Nack))
	{
		// nick nack pady wack
		X_ALIGNED_SYMBOL(char allocBuf[core::bitUtil::RoundUpToMultiple(MAX_MTU_SIZE, 256u)], 16) = {};

		core::StackAllocator allocator(allocBuf, allocBuf + sizeof(allocBuf));

		typedef core::MemoryArena<
			core::StackAllocator,
			core::SingleThreadPolicy,
			core::NoBoundsChecking,
			core::NoMemoryTracking,
			core::NoMemoryTagging
		> StackArena;

		// we don't get many of these sluts.
		StackArena arena(&allocator, "NackArena");
		DataGramNumberRangeList incomingNacks(&arena);

		if (!incomingNacks.fromBitStream(bs)) {
			X_ERROR("NetRel", "Failed to process incomming nacks's");
			return false;
		}

		for (auto& nackRange : incomingNacks)
		{
			// mark all the msg's for resend immediatly.




		}
	}
	else
	{
		// I GOT IT !
		// we ack for unreliable also, but never resend if no ack recived,
		addAck(dgh.number);

		// we can have multiple goat packets in a single MTU.
		// so we keep processing the bitStream untill it's empty.

		ReliablePacket* pPacket = packetFromBS(bs, time);
		while (pPacket)
		{
			// i don't trust you!

			if (pPacket->reliability == PacketReliability::ReliableSequenced ||
				pPacket->reliability == PacketReliability::ReliableOrdered ||
				pPacket->reliability == PacketReliability::UnReliableSequenced)
			{
				if (pPacket->orderingChannel > MAX_ORDERED_STREAMS)
				{
					// bitch who you think you are.
					goto tryNextPacket;
				}
			}




			recivedPackets_.push(pPacket);

		tryNextPacket:
			pPacket = packetFromBS(bs, time);
		}

		X_ASSERT(bs.isEos(), "Unprocessed bytes")(bs.isEos(), bs.size(), bs.capacity());
	}

	return true;
}

void ReliabilityLayer::update(core::FixedBitStreamBase& bs, NetSocket& socket, SystemAdd& systemAddress, int32_t MTUSize,
	core::TimeVal time)
{
	bs.reset();

	if ((lastBSPUpdate_ + core::TimeVal::fromMS(1000)) < time)
	{
		for (uint32_t i = 0; i < NetStatistics::Metric::ENUM_COUNT; i++)
		{
			bps_[i].update(time);
		}

		lastBSPUpdate_ = time;
	}

	const int32_t bitsPerSecondLimit = vars_.connectionBSPLimit();

	// HELLLLOOOO annnnyboddy.
	if (hasTimedOut(time)) {
		X_ERROR("NetRel", "Connection timed out");
		connectionDead_ = true;
		return;
	}

	// send acks.
	if (acks_.isNotEmpty()) {
		sendACKs(socket, bs, systemAddress, time);
	}

	// send naks.
	if (naks_.isNotEmpty()) {
		sendNAKs(socket, bs, systemAddress, time);
	}

	// resend packets.


	// send new packets.



	while (outGoingPackets_.isNotEmpty())
	{
		ReliablePacket* pPacket = outGoingPackets_.peek();
		outGoingPackets_.pop();

		// send it like a slut.
		bs.reset();

		DatagramHdr dgh;
		dgh.number = 0;
		dgh.flags.Clear();

		dgh.writeToBitStream(bs);
		pPacket->writeToBitStream(bs);


		sendBitStream(socket, bs, systemAddress, time);

		freePacket(pPacket);
	}
}

// called from peer to get recived packets back from ReliabilityLayer.
bool ReliabilityLayer::recive(PacketData& dataOut)
{
	if (recivedPackets_.isEmpty()) {
		return false;
	}

	ReliablePacket* pPacket = recivedPackets_.peek();
	dataOut.pData = pPacket->pData;
	dataOut.numBits = pPacket->dataBitLength;
	recivedPackets_.pop();
	return true;
}

bool ReliabilityLayer::hasTimedOut(core::TimeVal time)
{
	core::TimeVal elapsed = time - timeLastDatagramArrived_;
	
	if (elapsed > core::TimeVal::fromMS(vars_.defaultTimeoutMS()))
	{
		return true;
	}

	return false;
}

void ReliabilityLayer::sendACKs(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAdd& systemAddress, core::TimeVal time)
{
	// we want to pack all the acks into packets.
	const BitSizeT maxPacketBytes = MTUSize_ - sizeof(DatagramHdr);
	const BitSizeT maxPacketBits = core::bitUtil::bytesToBits(maxPacketBytes);

	while (acks_.isNotEmpty())
	{
		bs.reset();

		DatagramHdr dgh;
		dgh.number = 0;
		dgh.flags.Set(DatagramFlag::Nack);
		dgh.writeToBitStream(bs);

		// return instead of infinate loop, but if this happens dunno how we'd recover :Z
		// but it never 'should' happen.
		if (acks_.writeToBitStream(bs, maxPacketBits, true) == 0) {
			X_ERROR("NetRel", "Failed to write any ack's to stream");
			return;
		}

		// Weeeeeee!
		sendBitStream(socket, bs, systemAddress, time);
	}
}

void ReliabilityLayer::sendNAKs(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAdd& systemAddress, core::TimeVal time)
{
	// we want to pack all the acks into packets.
	const BitSizeT maxPacketBytes = MTUSize_ - sizeof(DatagramHdr);
	const BitSizeT maxPacketBits = core::bitUtil::bytesToBits(maxPacketBytes);

	while (naks_.isNotEmpty())
	{
		bs.reset();

		DatagramHdr dgh;
		dgh.number = 0;
		dgh.flags.Set(DatagramFlag::Nack);
		dgh.writeToBitStream(bs);

		if (naks_.writeToBitStream(bs, maxPacketBits, true) == 0) {
			X_ERROR("NetRel", "Failed to write any nack's to stream");
			return;
		}

		sendBitStream(socket, bs, systemAddress, time);
	}
}

void ReliabilityLayer::sendBitStream(NetSocket& socket, core::FixedBitStreamBase& bs, SystemAdd& systemAddress, core::TimeVal time)
{
	bps_[NetStatistics::Metric::ActualBytesSent].add(time, bs.sizeInBytes());

	SendParameters sp;
	sp.setData(bs);
	sp.systemAddress = systemAddress;
	socket.send(sp);
}

ReliablePacket* ReliabilityLayer::packetFromBS(core::FixedBitStreamBase& bs, core::TimeVal time)
{
	// meow.
	if (bs.isEos()) {
		return nullptr;
	}

	ReliablePacket* pPacket = allocPacket();
	pPacket->creationTime = time;
	if (!pPacket->fromBitStream(bs)) {
		freePacket(pPacket);
		return nullptr;
	}

	return pPacket;
}

ReliablePacket* ReliabilityLayer::allocPacket(void)
{
	return X_NEW(ReliablePacket, packetPool_, "RelPacket");
}

void ReliabilityLayer::freePacket(ReliablePacket* pPacker)
{
	X_DELETE(pPacker, packetPool_);
}

void ReliabilityLayer::getStatistics(NetStatistics& stats) const
{
	for (uint32_t i = 0; i < NetStatistics::Metric::ENUM_COUNT; i++)
	{
		stats.lastSecondMetrics[i] = bps_[i].getBPS();
		stats.runningMetrics[i] = bps_[i].getTotal();
	}

	stats.msgInSendBuffers = msgInSendBuffers_;
	stats.bytesInSendBuffers = bytesInSendBuffers_;

	stats.packetLossLastSecond = 0.f;
	stats.packetLossTotal = 0.f;

	stats.isLimitedByCongestionControl = false;
	stats.isLimitedByOutgoingBadwidthLimit = false;
}


X_NAMESPACE_END