#pragma once

#include "IPrimativeContext.h"

X_NAMESPACE_BEGIN(engine)

class PrimativeContext : public IPrimativeContext
{
public:
	typedef Vertex_P3F_T2F_C4B PrimVertex;

	struct PrimFlagBitMasks
	{
		enum Enum : uint32_t
		{
			TextureIdShift = 0,
			TextureIdMask = 0xFFFFF << TextureIdShift,

			PrimTypeShift = 20,
			PrimTypeMask = 0x7 << PrimTypeShift,
		};
	};

	static_assert(PrimitiveType::ENUM_COUNT < 7, "Primt enum count don't fit in prim type mask");

	struct PrimRenderFlags
	{
		X_INLINE PrimRenderFlags(PrimitiveType::Enum type, texture::TexID textureId);

		X_INLINE bool operator ==(const PrimRenderFlags& rhs) const;
		X_INLINE bool operator !=(const PrimRenderFlags& rhs) const;

		X_INLINE operator uint32_t();
		X_INLINE operator const uint32_t() const;

	private:
		uint32_t flags;
	};

	struct PushBufferEntry
	{
		PushBufferEntry() = default;

		X_INLINE PushBufferEntry(uint32 numVertices, uint32 vertexOffs,
			PrimRenderFlags flags);


		uint32 numVertices;
		uint32 vertexOffs;
		PrimRenderFlags flags;
	};

	X_ENSURE_SIZE(PushBufferEntry, 16); // not important, just ensuring padd correct.

	typedef core::Array<PushBufferEntry> PushBufferArr;
	typedef core::Array<const PushBufferEntry*> SortedPushBufferArr;
	typedef core::Array<PrimVertex> VertexArr;

public:
	PrimativeContext(core::MemoryArenaBase* arena);
	~PrimativeContext() X_OVERRIDE;

	void reset(void) X_FINAL;
	
	bool isEmpty(void) const;
	void getSortedBuffer(SortedPushBufferArr& sortedPushBuffer) const;
	const PushBufferArr& getUnsortedBuffer(void) const;
	const VertexArr& getVerts(void) const;

	void drawTextQueued(Vec3f pos, const render::XDrawTextInfo& ti, const char* pText) X_FINAL;

private:
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type, texture::TexID textureId) X_FINAL;
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type) X_FINAL;

private:
	PushBufferArr pushBufferArr_;
	VertexArr vertexArr_;
};

X_NAMESPACE_END

#include "PrimativeContext.inl"