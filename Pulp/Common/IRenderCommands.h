#pragma once


#include <Math\VertexFormats.h>

X_NAMESPACE_BEGIN(render)

typedef uintptr_t VertexBufferHandle;
typedef uintptr_t IndexBufferHandle;
typedef uintptr_t ConstantBufferHandle;
typedef uintptr_t StateHandle;

static const uintptr_t INVALID_BUF_HANLDE = 0;

namespace Commands
{
	X_DECLARE_ENUM(Command)(
		DRAW, 
		DRAW_INDEXED, 
		COPY_CONST_BUF_DATA,
		COPY_INDEXES_BUF_DATA,
		COPY_VERTEX_BUF_DATA
	);

	struct Draw
	{
		static const Command::Enum CMD = Command::DRAW;

		uint32_t vertexCount;
		uint32_t startVertex;

		StateHandle stateHandle;
		VertexBufferHandle vertexBuffers[VertexStream::ENUM_COUNT];
	};

	struct DrawIndexed
	{
		static const Command::Enum CMD = Command::DRAW_INDEXED;

		uint32_t indexCount;
		uint32_t startIndex;
		uint32_t baseVertex;

		StateHandle stateHandle;
		VertexBufferHandle vertexBuffers[VertexStream::ENUM_COUNT];
		IndexBufferHandle indexBuffer;
	};

	struct CopyConstantBufferData
	{
		static const Command::Enum CMD = Command::COPY_CONST_BUF_DATA;

		ConstantBufferHandle constantBuffer;
		void* pData;
		uint32_t size;
	};

	// these should only be used for updating not init of buf data.
	// the buffers must be none IMMUTABLE
	struct CopyIndexBufferData
	{
		static const Command::Enum CMD = Command::COPY_INDEXES_BUF_DATA;

		IndexBufferHandle indexBuffer;
		void* pData; // you own this, safe to clear after submitCommandPackets
		uint32_t size;
	};

	struct CopyVertexBufferData
	{
		static const Command::Enum CMD = Command::COPY_VERTEX_BUF_DATA;

		VertexBufferHandle vertexBuffer;
		void* pData; // you own this, safe to clear after submitCommandPackets
		uint32_t size;
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