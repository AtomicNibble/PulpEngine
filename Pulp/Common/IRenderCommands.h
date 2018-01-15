#pragma once

#include <IRender.h>

X_NAMESPACE_BEGIN(render)


namespace Commands
{
	X_DECLARE_ENUM(Command)(
		NOP, // used for simplyfing chaining logic.
		DRAW,
		DRAW_INDEXED,
		DRAW_INSTANCED,
		DRAW_INSTANCED_INDEXED, 
		COPY_CONST_BUF_DATA,
		COPY_INDEXES_BUF_DATA,
		COPY_VERTEX_BUF_DATA,
		UPDATE_TEXTUTE_BUF_DATA,
		UPDATE_TEXTUTE_SUB_BUF_DATA,
		CLEAR_DEPTH_STENCIL,
		CLEAR_COLOR
	);

	// how to pass these states in nice way.
	// i don't really want to create handles for them as these are just arrays of handles.
	// so maybe storing this info in the bucket is just best way?
	// 
	// 

	X_PACK_PUSH(2)
	X_DISABLE_WARNING(4201)
	struct ResourceStateBase
	{
		static const uint32_t MAX_CONST_BUFFERS = render::MAX_CONST_BUFFERS_BOUND;
		static const uint32_t MAX_TEX_STATES = render::MAX_TEXTURES_BOUND;
		static const uint32_t MAX_SAMPLER_STATES = render::MAX_TEXTURES_BOUND;
		static const uint32_t MAX_BUFFER_STATES = render::MAX_BUFFERS_BOUND; 

	public:
		X_INLINE int8_t getNumTextStates(void) const;
		X_INLINE int8_t getNumSamplers(void) const;
		X_INLINE int8_t getNumCBs(void) const;
		X_INLINE int8_t getNumBuffers(void) const;

		X_INLINE bool anySet(void) const;

		X_INLINE TextureState* getTexStates(void);
		X_INLINE SamplerState* getSamplers(void);
		X_INLINE ConstantBufferHandle* getCBs(void);
		X_INLINE BufferState* getBuffers(void);

		X_INLINE const TextureState* getTexStates(void) const;
		X_INLINE const SamplerState* getSamplers(void) const;
		X_INLINE const ConstantBufferHandle* getCBs(void) const;
		X_INLINE const BufferState* getBuffers(void) const;

		X_INLINE int32_t getTotalSize(void) const;
		X_INLINE int32_t getStateSize(void) const;
		X_INLINE static constexpr int32_t getMaxStateSize(void);

		X_INLINE const uint8_t* getDataStart(void) const;
		X_INLINE uint8_t* getDataStart(void);

		X_INLINE const TextureState* getTexStates(const void* pBase) const;
		X_INLINE const SamplerState* getSamplers(const void* pBase) const;
		X_INLINE const ConstantBufferHandle* getCBs(const void* pBase) const;
		X_INLINE const BufferState* getBuffers(const void* pBase) const;

	protected:
		union {
			struct {
				int8_t numTextStates;
				int8_t numSamplers;
				int8_t numCbs;
				int8_t numBuffers; // stuff like fluffly mittens.
			};
			uint32_t val;
		};
	};
	X_PACK_POP
	X_ENABLE_WARNING(4201)

	X_ENSURE_SIZE(ResourceStateBase, 4);

	X_INLINE constexpr int32_t ResourceStateBase::getMaxStateSize(void)
	{
		return sizeof(ResourceStateBase) +
			(sizeof(render::TextureState) * MAX_TEX_STATES) +
			(sizeof(render::SamplerState) * MAX_SAMPLER_STATES) +
			(sizeof(render::ConstantBufferHandle) * MAX_CONST_BUFFERS) + 
			(sizeof(render::BufferState) * MAX_BUFFER_STATES);
	}
	

	X_INLINE int8_t ResourceStateBase::getNumTextStates(void) const
	{
		return numTextStates;
	}

	X_INLINE int8_t ResourceStateBase::getNumSamplers(void) const
	{
		return numSamplers;
	}

	X_INLINE int8_t ResourceStateBase::getNumCBs(void) const
	{
		return numCbs;
	}

	X_INLINE int8_t ResourceStateBase::getNumBuffers(void) const
	{
		return numBuffers;
	}

	X_INLINE bool ResourceStateBase::anySet(void) const
	{
		return val != 0;
	}

	X_INLINE TextureState* ResourceStateBase::getTexStates(void)
	{
		return reinterpret_cast<TextureState*>(getDataStart());
	}

	X_INLINE SamplerState* ResourceStateBase::getSamplers(void)
	{
		uint8_t* pStart = getDataStart();
		pStart += (sizeof(TextureState) * numTextStates);
		return reinterpret_cast<SamplerState*>(pStart);
	}

	X_INLINE ConstantBufferHandle* ResourceStateBase::getCBs(void)
	{
		uint8_t* pStart = getDataStart();
		pStart += (sizeof(TextureState) * numTextStates);
		pStart += (sizeof(SamplerState) * numSamplers);
		return reinterpret_cast<ConstantBufferHandle*>(pStart);
	}

	X_INLINE BufferState* ResourceStateBase::getBuffers(void)
	{
		uint8_t* pStart = getDataStart();
		pStart += (sizeof(TextureState) * numTextStates);
		pStart += (sizeof(SamplerState) * numSamplers);
		pStart += (sizeof(ConstantBufferHandle) * numCbs);
		return reinterpret_cast<BufferState*>(pStart);
	}

	X_INLINE const TextureState* ResourceStateBase::getTexStates(void) const
	{
		return reinterpret_cast<const TextureState*>(getDataStart());
	}

	X_INLINE const SamplerState* ResourceStateBase::getSamplers(void) const
	{
		const uint8_t* pStart = getDataStart();
		pStart += (sizeof(TextureState) * numTextStates);
		return reinterpret_cast<const SamplerState*>(pStart);
	}


	X_INLINE const ConstantBufferHandle* ResourceStateBase::getCBs(void) const
	{
		const uint8_t* pStart = getDataStart();
		pStart += (sizeof(TextureState) * numTextStates);
		pStart += (sizeof(SamplerState) * numSamplers);
		return reinterpret_cast<const ConstantBufferHandle*>(pStart);
	}

	X_INLINE const BufferState* ResourceStateBase::getBuffers(void) const
	{
		const uint8_t* pStart = getDataStart();
		pStart += (sizeof(TextureState) * numTextStates);
		pStart += (sizeof(SamplerState) * numSamplers);
		pStart += (sizeof(ConstantBufferHandle) * numCbs);
		return reinterpret_cast<const BufferState*>(pStart);
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
		size += (sizeof(SamplerState) * numSamplers);
		size += (sizeof(ConstantBufferHandle) * numCbs);
		size += (sizeof(BufferState) * numBuffers);
		return size;
	}

	X_INLINE int32_t ResourceStateBase::getStateSize(void) const
	{
		int32_t size = 0;
		size += (sizeof(TextureState) * numTextStates);
		size += (sizeof(SamplerState) * numSamplers);
		size += (sizeof(ConstantBufferHandle) * numCbs);
		size += (sizeof(BufferState) * numBuffers);
		return size;
	}

	X_INLINE const TextureState* ResourceStateBase::getTexStates(const void* pBase) const
	{
		return reinterpret_cast<const TextureState*>(pBase);
	}

	X_INLINE const SamplerState* ResourceStateBase::getSamplers(const void* pBase) const
	{
		const uint8_t* pStart = reinterpret_cast<const uint8_t*>(pBase);
		pStart += (sizeof(TextureState) * numTextStates);
		return reinterpret_cast<const SamplerState*>(pStart);
	}

	X_INLINE const ConstantBufferHandle* ResourceStateBase::getCBs(const void* pBase) const
	{
		const uint8_t* pStart = reinterpret_cast<const uint8_t*>(pBase);
		pStart += (sizeof(TextureState) * numTextStates);
		pStart += (sizeof(SamplerState) * numSamplers);
		return reinterpret_cast<const ConstantBufferHandle*>(pStart);
	}

	X_INLINE const BufferState* ResourceStateBase::getBuffers(const void* pBase) const
	{
		const uint8_t* pStart = reinterpret_cast<const uint8_t*>(pBase);
		pStart += (sizeof(TextureState) * numTextStates);
		pStart += (sizeof(SamplerState) * numSamplers);
		pStart += (sizeof(ConstantBufferHandle) * numCbs);
		return reinterpret_cast<const BufferState*>(pStart);
	}

	// not sure if i want to pack these down close or have each command start aligned.
	// I currently support them been 8 bute aligned.
	X_PACK_PUSH(1)

	struct CmdBase
	{
		
	};

	X_ENSURE_SIZE(CmdBase, 1);

	struct Nop : public CmdBase
	{
		static const Command::Enum CMD = Command::NOP;

	};

	X_ENSURE_SIZE(Nop, 1);

	struct Draw : public CmdBase
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

	struct DrawIndexed : public CmdBase
	{
		static const Command::Enum CMD = Command::DRAW_INDEXED;

		uint32_t indexCount;
		uint32_t startIndex;
		uint32_t baseVertex;

		StateHandle stateHandle;
		VertexBufferHandleArr vertexBuffers;
		IndexBufferHandle indexBuffer;
		ResourceStateBase resourceState;
	};

	struct DrawInstanced : public CmdBase
	{
		static const Command::Enum CMD = Command::DRAW_INSTANCED;

		uint32_t vertexCountPerInstance;
		uint32_t instanceCount;
		uint32_t startVertexLocation;
		uint32_t startInstanceLocation;

		StateHandle stateHandle;
		VertexBufferHandleArr vertexBuffers;
		ResourceStateBase resourceState;
	};

	struct DrawInstancedIndexed : public CmdBase
	{
		static const Command::Enum CMD = Command::DRAW_INSTANCED_INDEXED;

		uint32_t indexCountPerInstance;
		uint32_t instanceCount;
		uint32_t startIndexLocation;
		uint32_t baseVertexLocation;
		uint32_t startInstanceLocation;

		StateHandle stateHandle;
		VertexBufferHandleArr vertexBuffers;
		IndexBufferHandle indexBuffer;
		ResourceStateBase resourceState;
	};



	struct CopyConstantBufferData : public CmdBase
	{
		static const Command::Enum CMD = Command::COPY_CONST_BUF_DATA;

		ConstantBufferHandle constantBuffer;
		const void* pData;
		uint32_t size;
	};

	// these should only be used for updating not init of buf data.
	// the buffers must be none IMMUTABLE
	struct CopyIndexBufferData : public CmdBase
	{
		static const Command::Enum CMD = Command::COPY_INDEXES_BUF_DATA;

		IndexBufferHandle indexBuffer;
		const void* pData; // you own this, safe to clear after submitCommandPackets
		uint32_t size;
		uint32_t dstOffset;
	};

	struct CopyVertexBufferData : public CmdBase
	{
		static const Command::Enum CMD = Command::COPY_VERTEX_BUF_DATA;

		VertexBufferHandle vertexBuffer;
		const void* pData; // you own this, safe to clear after submitCommandPackets
		uint32_t size;
		uint32_t dstOffset;
	};

	struct CopyTextureBufferData : public CmdBase
	{
		static const Command::Enum CMD = Command::UPDATE_TEXTUTE_BUF_DATA;

		texture::TexID textureId;
		const void* pData; // you own this, safe to clear after submitCommandPackets
		uint32_t size;
	};

	struct CopyTextureSubRegionBufferData : public CmdBase
	{
		static const Command::Enum CMD = Command::UPDATE_TEXTUTE_SUB_BUF_DATA;

		texture::TexID textureId;
		const void* pData; // you own this, safe to clear after submitCommandPackets
		uint32_t size;
		Recti rect; // the rect to update.
	};

	struct ClearDepthStencil : public CmdBase
	{
		static const Command::Enum CMD = Command::CLEAR_DEPTH_STENCIL;

		IPixelBuffer* pDepthBuffer;
	};

	struct ClearColor : public CmdBase
	{
		static const Command::Enum CMD = Command::CLEAR_COLOR;

		IPixelBuffer* pColorBuffer;
	};

	//~

	X_PACK_POP


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
	/*
		A Packet is like
		
		template<typename CmdT>
		struct Packet;
		{
			Packet* pNext;
			Command::Enum cmd;
			CmdT;  <-- we return pointers to this, hence the he helpers taking Cmd that offset back.
		};
	*/

	typedef void* Packet;
	typedef Commands::CmdBase CmdBase;
	typedef Commands::Command Command;
	const size_t OFFSET_NEXT_COMMAND_PACKET = 0u;
	const size_t OFFSET_COMMAND_TYPE = OFFSET_NEXT_COMMAND_PACKET + sizeof(Packet);
	const size_t OFFSET_COMMAND = OFFSET_COMMAND_TYPE + sizeof(Command::Enum);

	template <typename CommandT>
	X_INLINE size_t getPacketSize(size_t auxMemorySize);

	Packet* getNextCommandPacket(Packet pPacket);

	X_INLINE Packet* getNextCommandPacket(CmdBase* command);

	Command::Enum* getCommandType(Packet pPacket);

	template <typename CommandT>
	X_INLINE CommandT* getCommand(Packet packet);

	template <typename CommandT>
	X_INLINE char* getAuxiliaryMemory(CommandT* command);
	template <typename CommandT>
	X_INLINE const char* getAuxiliaryMemory(const CommandT* command);

	void storeNextCommandPacket(Packet pPacket, Packet nextPacket);

	X_INLINE void storeNextCommandPacket(CmdBase* command, Packet nextPacket);

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

	X_INLINE Packet* getNextCommandPacket(CmdBase* command)
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

	X_INLINE void storeNextCommandPacket(CmdBase* command, Packet nextPacket)
	{
		*getNextCommandPacket(command) = nextPacket;
	}

	X_INLINE Packet* getNextCommandPacket(Packet pPacket)
	{
		return union_cast<Packet*>(reinterpret_cast<char*>(pPacket) + OFFSET_NEXT_COMMAND_PACKET);
	}

	X_INLINE Command::Enum* getCommandType(Packet pPacket)
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