#pragma once


X_NAMESPACE_BEGIN(render)


 // -------------------------------------------------------

X_INLINE void CommandBucketBase::appendRenderTarget(IRenderTarget* pRTV)
{
	rtvs_.push_back(pRTV);
}



X_INLINE const Matrix44f& CommandBucketBase::getViewMatrix(void) const
{
	return view_;
}

X_INLINE const Matrix44f& CommandBucketBase::getProjMatrix(void) const
{
	return proj_;
}

X_INLINE const XViewPort& CommandBucketBase::getViewport(void) const
{
	return viewport_;
}

X_INLINE const CommandBucketBase::RenderTargetsArr& CommandBucketBase::getRTVS(void) const
{
	return rtvs_;
}

X_INLINE const CommandBucketBase::SortedIdxArr& CommandBucketBase::getSortedIdx(void) const
{
	return sortedIdx_;
}

X_INLINE const CommandBucketBase::PacketArr& CommandBucketBase::getPackets(void) const
{
	return packets_;
}

// -------------------------------------------------------

template <typename CommandT>
X_INLINE CommandPacket::Packet CmdPacketAllocator::create(size_t threadIdx, size_t auxMemorySize)
{
	static_assert(core::compileTime::IsPOD<CommandT>::Value, "Command packet type must be POD");

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
X_INLINE CommandBucket<KeyT>::ThreadSlotInfo::ThreadSlotInfo() :
	offset(0),
	remaining(0) 
{
}

template <typename KeyT>
template <typename CommandT>
X_INLINE CommandT* CommandBucket<KeyT>::addCommand(Key key, size_t auxMemorySize)
{
	static_assert(core::compileTime::IsPOD<CommandT>::Value, "Command packet type must be POD");

	const auto threadIdx = packetAlloc_.getThreadIdx();

	CommandPacket::Packet pPacket = packetAlloc_.create<CommandT>(threadIdx, auxMemorySize);

	// store key and pointer to the data
	{
		int32_t remaining = threadSlotsInfo_[threadIdx].remaining;
		int32_t offset = threadSlotsInfo_[threadIdx].offset;

		if (remaining == 0)
		{
			// check if we have space.
			X_ASSERT(current_ < safe_static_cast<int32_t>(keys_.capacity()), "CmdBucket is full")(current_, keys_.capacity());

			// no more storage in this block remaining, get new one
			offset = current_ += FETCH_SIZE;
			remaining = FETCH_SIZE;

			// write back
			threadSlotsInfo_[threadIdx].offset = offset;
		}

		const int32_t current = offset + (FETCH_SIZE - remaining);
		keys_[current] = key;
		packets_[current] = pPacket;
		--remaining;

		// write back
		threadSlotsInfo_[threadIdx].remaining = remaining;
	}

	CommandPacket::storeNextCommandPacket(pPacket, nullptr);
	CommandPacket::storeCommandType(pPacket, CommandT::CMD);

	return CommandPacket::getCommand<CommandT>(pPacket);
}


template <typename KeyT>
template <typename CommandT, typename ParentCmdT>
X_INLINE CommandT* CommandBucket<KeyT>::appendCommand(ParentCmdT* pCommand, size_t auxMemorySize)
{
	static_assert(core::compileTime::IsPOD<CommandT>::Value, "Command packet type must be POD");

	const auto threadIdx = packetAlloc_.getThreadIdx();

	CommandPacket::Packet pPacket = packetAlloc_.create<CommandT>(threadIdx, auxMemorySize);

	// append this command to the given one
	CommandPacket::storeNextCommandPacket<ParentCmdT>(pCommand, pPacket);
				   
	CommandPacket::storeNextCommandPacket(pPacket, nullptr);
	CommandPacket::storeCommandType(pPacket, CommandT::CMD);

	return CommandPacket::getCommand<CommandT>(pPacket);
}

template <typename KeyT>
X_INLINE const typename CommandBucket<KeyT>::KeyArr& CommandBucket<KeyT>::getKeys(void)
{
	return keys_;
}

X_NAMESPACE_END