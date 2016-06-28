#pragma once


X_NAMESPACE_BEGIN(engine)

template <typename CommandT>
X_INLINE size_t CommandPacket::getPacketSize(size_t auxMemorySize)
{
	return OFFSET_COMMAND + sizeof(CommandT) + auxMemorySize;
}

template <typename CommandT>
X_INLINE CommandPacket::Packet* CommandPacket::getNextCommandPacket(CommandT* command)
{
	return union_cast<Packet*>(reinterpret_cast<char*>(command) - OFFSET_COMMAND + OFFSET_NEXT_COMMAND_PACKET);
}

template <typename CommandT>
X_INLINE CommandT* CommandPacket::getCommand(Packet packet)
{
	return union_cast<CommandT*>(reinterpret_cast<char*>(packet) + OFFSET_COMMAND);
}

template <typename CommandT>
X_INLINE char* CommandPacket::getAuxiliaryMemory(CommandT* command)
{
	return reinterpret_cast<char*>(command) + sizeof(CommandT);
}

template <typename CommandT>
X_INLINE void CommandPacket::storeNextCommandPacket(CommandT* command, Packet nextPacket)
{
	*commandPacket::GetNextCommandPacket<CommandT>(command) = nextPacket;
}





// -------------------------------------------------------


template <typename CommandT>
X_INLINE CommandPacket::Packet CmdPacketAllocator::create(size_t threadIdx, size_t auxMemorySize)
{
	// we have linera allocators for each thread.
	ThreadAllocator& allocator = *allocators_[threadIdx];

	return X_NEW_ARRAY(uint8_t, CommandPacket::getPacketSize<CommandT>(auxMemorySize), &allocator.arena_, "CmdPacket");
}


X_INLINE size_t CmdPacketAllocator::getThreadIdx(void)
{
	uint32_t threadId = core::Thread::GetCurrentID();
	size_t idx = 0;

	for (auto id : threadIdToIndex_) {
		if (id == threadId) {
			return idx;
		}
		++idx;
	}

	X_ASSERT_UNREACHABLE();
	return 0;
}


// -------------------------------------------------------


template <typename KeyT>
template <typename CommandT>
X_INLINE CommandT* CommandBucket<KeyT>::addCommand(Key key, size_t auxMemorySize)
{
	size_t threadIdx = packetAlloc_.getThreadIdx();

	Packet pPacket = packetAlloc_.create<CommandT>(threadIdx, auxMemorySize);

	// store key and pointer to the data
	{
		int32_t remaining = slotsLeft_[threadIdx].val;
		int32_t offset = offsets_[threadIdx].val;

		if (remaining == 0)
		{
			// no more storage in this block remaining, get new one
			offset = current_ += FETCH_SIZE;
			remaining = FETCH_SIZE;

			// write back
			offsets_[threadIdx].val = offset;
		}

		const int32_t current = offset + (FETCH_SIZE - remaining);
		keys_[current] = key;
		packets_[current] = pPacket;
		--remaining;

		// write back
		slotsLeft_[threadIdx].val = remaining;
	}

	CommandPacket::storeNextCommandPacket(pPacket, nullptr);
	CommandPacket::storeBackendDispatchFunction(pPacket, CommandT::DISPATCH_FUNCTION);

	return CommandPacket::getCommand<CommandT>(pPacket);
}


template <typename KeyT>
template <typename CommandT, typename ParentCmdT>
X_INLINE CommandT* CommandBucket<KeyT>::appendCommand(ParentCmdT* pCommand, size_t auxMemorySize)
{
	uint32_t threadIdx = packetAlloc_.getThreadIdx();

	Packet pPacket = packetAlloc_.create<CommandT>(threadIdx, auxMemorySize);

	// append this command to the given one
	CommandPacket::storeNextCommandPacket<ParentCmdT>(pCommand, pPacket);
				   
	CommandPacket::storeNextCommandPacket(packet, nullptr);
	CommandPacket::storeBackendDispatchFunction(pPacket, CommandT::DISPATCH_FUNCTION);

	return CommandPacket::getCommand<CommandT>(pPacket);
}


X_NAMESPACE_END