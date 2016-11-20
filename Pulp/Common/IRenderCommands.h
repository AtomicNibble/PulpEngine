#pragma once

#include <IRender.h>

X_NAMESPACE_BEGIN(render)


namespace Commands
{
	X_DECLARE_ENUM(Command)(
		DRAW,
		DRAW_INDEXED,
		COPY_CONST_BUF_DATA,
		COPY_INDEXES_BUF_DATA,
		COPY_VERTEX_BUF_DATA,
		UPDATE_TEXTUTE_BUF_DATA,
		UPDATE_TEXTUTE_SUB_BUF_DATA
	);

	// how to pass these states in nice way.
	// i don't really want to create handles for them as these are just arrays of handles.
	// so maybe storing this info in the bucket is just best way?
	// 
	// 

#if 1

	X_PACK_PUSH(2)
	X_DISABLE_WARNING(4201)
	struct ResourceStateBase
	{
		X_INLINE int8_t getNumTextStates(void) const;
		X_INLINE int8_t getNumCBs(void) const;

		X_INLINE bool anySet(void) const;

		X_INLINE TextureState* getTexStates(void);
		X_INLINE ConstantBufferHandle* getCBs(void);

		X_INLINE const TextureState* getTexStates(void) const;
		X_INLINE const ConstantBufferHandle* getCBs(void) const;

		X_INLINE int32_t getTotalSize(void) const;
		X_INLINE int32_t getStateSize(void) const;

		X_INLINE const uint8_t* getDataStart(void) const;
		X_INLINE uint8_t* getDataStart(void);

	protected:
		union {
			struct {
				int8_t numTextStates;
				int8_t numCbs;
			};
			uint16_t val;
		};
	};
	X_PACK_POP
	X_ENABLE_WARNING(4201)

	X_ENSURE_SIZE(ResourceStateBase, 2);

	X_INLINE int8_t ResourceStateBase::getNumTextStates(void) const
	{
		return numTextStates;
	}

	X_INLINE int8_t ResourceStateBase::getNumCBs(void) const
	{
		return numCbs;
	}

	X_INLINE bool ResourceStateBase::anySet(void) const
	{
		return val != 0;
	}

	X_INLINE TextureState* ResourceStateBase::getTexStates(void)
	{
		return reinterpret_cast<TextureState*>(getDataStart());
	}


	X_INLINE ConstantBufferHandle* ResourceStateBase::getCBs(void)
	{
		uint8_t* pStart = getDataStart();
		pStart += (sizeof(TextureState) * numTextStates);
		return reinterpret_cast<ConstantBufferHandle*>(pStart);
	}

	X_INLINE const TextureState* ResourceStateBase::getTexStates(void) const
	{
		return reinterpret_cast<const TextureState*>(getDataStart());
	}

	X_INLINE const ConstantBufferHandle* ResourceStateBase::getCBs(void) const
	{
		const uint8_t* pStart = getDataStart();
		pStart += (sizeof(TextureState) * numTextStates);
		return reinterpret_cast<const ConstantBufferHandle*>(pStart);
	}

	X_INLINE const uint8_t* ResourceStateBase::getDataStart(void) const
	{
		return reinterpret_cast<const uint8_t*>(this + 1);
	}

	X_INLINE uint8_t* ResourceStateBase::getDataStart(void)
	{
		return reinterpret_cast<uint8_t*>(this + 1);
	}

	X_INLINE int32_t ResourceStateBase::getTotalSize(void) const
	{
		int32_t size = sizeof(ResourceStateBase);
		size += (sizeof(TextureState) * numTextStates);
		size += (sizeof(ConstantBufferHandle) * numCbs);
		return size;
	}

	X_INLINE int32_t ResourceStateBase::getStateSize(void) const
	{
		int32_t size = 0;
		size += (sizeof(TextureState) * numTextStates);
		size += (sizeof(ConstantBufferHandle) * numCbs);
		return size;
	}


#else

	X_DISABLE_WARNING(4201)

	struct ResourceStateBase
	{
		union {
			struct {
				int8_t numTextStates;
				int8_t numVertexBufs;
				int8_t numCbs;
				int8_t _pad;
			};
			uint32_t val;
		};
	};
	X_ENABLE_WARNING(4201)

	template<size_t numTex, size_t numVBs, size_t numCBs>
	struct ResourceState : public ResourceStateBase
	{
		static const uint32_t STATE_DATA_SIZE =
			(sizeof(TextureState) * numTex) + 
			(sizeof(VertexBufferHandle) * numVBs) +
			(sizeof(ConstantBufferHandle) * numCBs);

		X_INLINE const void* getData(void) const {
			return &tex;
		}

		TextureState tex[numTex];
		VertexBufferHandle vbs[numVBs];
		ConstantBufferHandle cbs[numCBs];
	};

	template<size_t numTex, size_t numVBs>
	struct ResourceState<numTex, numVBs, 0> : public ResourceStateBase
	{
		static const uint32_t STATE_DATA_SIZE =
			(sizeof(TextureState) * numTex) +
			(sizeof(VertexBufferHandle) * numVBs);

		X_INLINE const void* getData(void) const {
			return &tex;
		}

		TextureState tex[numTex];
		VertexBufferHandle vbs[numVBs];
	};


	template<size_t numTex, size_t numCBs>
	struct ResourceState<numTex, 0, numCBs> : public ResourceStateBase
	{
		static const uint32_t STATE_DATA_SIZE =
			(sizeof(TextureState) * numTex) +
			(sizeof(ConstantBufferHandle) * numCBs);

		X_INLINE const void* getData(void) const {
			return &tex;
		}

		TextureState tex[numTex];
		ConstantBufferHandle cbs[numCBs];
	};

	template<size_t numVBs, size_t numCBs>
	struct ResourceState<0, numVBs, numCBs> : public ResourceStateBase
	{
		static const uint32_t STATE_DATA_SIZE =
			(sizeof(VertexBufferHandle) * numVBs) +
			(sizeof(ConstantBufferHandle) * numCBs);

		X_INLINE const void* getData(void) const {
			return &vbs;
		}

		VertexBufferHandle vbs[numVBs];
		ConstantBufferHandle cbs[numCBs];
	};

	template<size_t numTex>
	struct ResourceState<numTex, 0, 0> : public ResourceStateBase
	{
		static const uint32_t STATE_DATA_SIZE =
			(sizeof(TextureState) * numTex);

		X_INLINE const void* getData(void) const {
			return &tex;
		}

		TextureState tex[numTex];
	};

	template<size_t numVBs>
	struct ResourceState<0, numVBs, 0> : public ResourceStateBase
	{
		static const uint32_t STATE_DATA_SIZE =
			(sizeof(VertexBufferHandle) * numVBs);

		X_INLINE const void* getData(void) const {
			return &vbs;
		}

		VertexBufferHandle vbs[numVBs];
	};

	template<size_t numCBs>
	struct ResourceState<0, 0, numCBs> : public ResourceStateBase
	{
		static const uint32_t STATE_DATA_SIZE =
			(sizeof(ConstantBufferHandle) * numCBs);

		X_INLINE const void* getData(void) const {
			return &vbs;
		}

		ConstantBufferHandle cbs[numCBs];
	};

#endif

	struct Draw
	{
		static const Command::Enum CMD = Command::DRAW;

		uint32_t vertexCount;
		uint32_t startVertex;


		StateHandle stateHandle;
		VertexBufferHandleArr vertexBuffers;
		// maybe i could make this in place and place the data in the aux buf.
		// requires copying of data but then the render pipeline can read from same cache line
		// i'll bench mark it.
		// more memory but better cache access is proberly faster.
		ResourceStateBase resourceState;
	};

	struct DrawIndexed
	{
		static const Command::Enum CMD = Command::DRAW_INDEXED;

		uint32_t indexCount;
		uint32_t startIndex;
		uint32_t baseVertex;

		StateHandle stateHandle;
		IndexBufferHandle indexBuffer;
		ResourceStateBase resourceState;

	};

	struct CopyConstantBufferData
	{
		static const Command::Enum CMD = Command::COPY_CONST_BUF_DATA;

		ConstantBufferHandle constantBuffer;
		const void* pData;
		uint32_t size;
	};

	// these should only be used for updating not init of buf data.
	// the buffers must be none IMMUTABLE
	struct CopyIndexBufferData
	{
		static const Command::Enum CMD = Command::COPY_INDEXES_BUF_DATA;

		IndexBufferHandle indexBuffer;
		const void* pData; // you own this, safe to clear after submitCommandPackets
		uint32_t size;
	};

	struct CopyVertexBufferData
	{
		static const Command::Enum CMD = Command::COPY_VERTEX_BUF_DATA;

		VertexBufferHandle vertexBuffer;
		const void* pData; // you own this, safe to clear after submitCommandPackets
		uint32_t size;
	};

	struct CopyTextureBufferData
	{
		static const Command::Enum CMD = Command::UPDATE_TEXTUTE_BUF_DATA;

		texture::TexID textureId;
		const void* pData; // you own this, safe to clear after submitCommandPackets
		uint32_t size;
	};

	struct CopyTextureSubRegionBufferData
	{
		static const Command::Enum CMD = Command::UPDATE_TEXTUTE_SUB_BUF_DATA;

		texture::TexID textureId;
		const void* pData; // you own this, safe to clear after submitCommandPackets
		uint32_t size;
		Recti rect; // the rect to update.
	};
	//~


	namespace Key
	{
		// this will be for creating the G buffer.
		// we will sort by depth first.
		// 
		// we also need the material id to support the number of materials we have.
		X_DECLARE_ENUM(Type)(DEPTH, PRIM);

		struct DepthPass
		{
			uint32_t depth : 18; // 0 - 262143
			uint32_t materialId : 14; // 16383 - the real id of the material.

		};

		struct Prim
		{
			uint32_t layer : 3; // 0 - layers
			uint32_t pad : 6; 
			uint32_t primType : 3; // 0 - layers
			uint32_t textureId : 20; // 0 - layers
			// rest unused.
		};

	}

	static_assert(core::compileTime::IsPOD<Draw>::Value, "Draw command must be POD");
	static_assert(core::compileTime::IsPOD<DrawIndexed>::Value, "DrawIndexed command must be POD");
	static_assert(core::compileTime::IsPOD<CopyConstantBufferData>::Value, "CopyConstantBufferData command must be POD");

} // namespace Commands


namespace CommandPacket
{
	typedef void* Packet;
	typedef Commands::Command Command;
	const size_t OFFSET_NEXT_COMMAND_PACKET = 0u;
	const size_t OFFSET_COMMAND_TYPE = OFFSET_NEXT_COMMAND_PACKET + sizeof(Packet);
	const size_t OFFSET_COMMAND = OFFSET_COMMAND_TYPE + sizeof(Command::Enum);

	template <typename CommandT>
	X_INLINE size_t getPacketSize(size_t auxMemorySize);

	Packet* getNextCommandPacket(Packet pPacket);

	template <typename CommandT>
	X_INLINE Packet* getNextCommandPacket(CommandT* command);

	Command::Enum* getCommandType(Packet pPacket);

	template <typename CommandT>
	X_INLINE CommandT* getCommand(Packet packet);

	template <typename CommandT>
	X_INLINE char* getAuxiliaryMemory(CommandT* command);
	template <typename CommandT>
	X_INLINE const char* getAuxiliaryMemory(const CommandT* command);

	void storeNextCommandPacket(Packet pPacket, Packet nextPacket);

	template <typename CommandT>
	X_INLINE void storeNextCommandPacket(CommandT* command, Packet nextPacket);

	void storeCommandType(Packet pPacket, Command::Enum dispatchFunction);
	const Packet loadNextCommandPacket(const Packet pPacket);
	const Command::Enum loadCommandType(const Packet pPacket);
	const void* loadCommand(const Packet pPacket);

} // namespace CommandPacket


namespace CommandPacket
{
	template <typename CommandT>
	X_INLINE size_t getPacketSize(size_t auxMemorySize)
	{
		return OFFSET_COMMAND + sizeof(CommandT) + auxMemorySize;
	}

	template <typename CommandT>
	X_INLINE Packet* getNextCommandPacket(CommandT* command)
	{
		return union_cast<Packet*>(reinterpret_cast<char*>(command) - OFFSET_COMMAND + OFFSET_NEXT_COMMAND_PACKET);
	}

	template <typename CommandT>
	X_INLINE CommandT* getCommand(Packet packet)
	{
		return union_cast<CommandT*>(reinterpret_cast<char*>(packet) + OFFSET_COMMAND);
	}

	template <typename CommandT>
	X_INLINE char* getAuxiliaryMemory(CommandT* command)
	{
		return reinterpret_cast<char*>(command) + sizeof(CommandT);
	}

	template <typename CommandT>
	X_INLINE const char* getAuxiliaryMemory(const CommandT* command)
	{
		return reinterpret_cast<const char*>(command) + sizeof(CommandT);
	}

	template <typename CommandT>
	X_INLINE void storeNextCommandPacket(CommandT* command, Packet nextPacket)
	{
		*getNextCommandPacket<CommandT>(command) = nextPacket;
	}

	X_INLINE Packet* CommandPacket::getNextCommandPacket(Packet pPacket)
	{
		return union_cast<Packet*>(reinterpret_cast<char*>(pPacket) + OFFSET_NEXT_COMMAND_PACKET);
	}

	X_INLINE Command::Enum* CommandPacket::getCommandType(Packet pPacket)
	{
		return union_cast<Command::Enum*>(reinterpret_cast<char*>(pPacket) + OFFSET_COMMAND_TYPE);
	}

	X_INLINE void storeNextCommandPacket(Packet pPacket, Packet nextPacket)
	{
		*getNextCommandPacket(pPacket) = nextPacket;
	}

	X_INLINE void storeCommandType(CommandPacket::Packet pPacket, Command::Enum type)
	{
		*getCommandType(pPacket) = type;
	}

	X_INLINE const Packet loadNextCommandPacket(const Packet pPacket)
	{
		return *getNextCommandPacket(pPacket);
	}

	X_INLINE const Command::Enum loadCommandType(const Packet pPacket)
	{
		return *getCommandType(pPacket);
	}

	X_INLINE const void* loadCommand(const Packet pPacket)
	{
		return reinterpret_cast<char*>(pPacket) + OFFSET_COMMAND;
	}

} // namespace CommandPacket


X_NAMESPACE_END