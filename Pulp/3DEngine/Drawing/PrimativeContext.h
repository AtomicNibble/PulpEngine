#pragma once

#include "IPrimativeContext.h"

X_NAMESPACE_BEGIN(engine)

class PrimativeContext : public IPrimativeContext
{
public:
	typedef Vertex_P3F_T2F_C4B PrimVertex;


	struct PrimRenderFlags
	{
		X_INLINE PrimRenderFlags(PrimitiveType::Enum type, texture::TexID textureId);


		X_INLINE bool operator ==(const PrimRenderFlags& rhs) const;
		X_INLINE bool operator !=(const PrimRenderFlags& rhs) const;

		X_INLINE operator uint64_t();
		X_INLINE operator const uint64_t() const;

		// could store both type and flags in a 32bit int if I decide I need to store flags.
		// we sort by type then texture.
		X_DISABLE_WARNING(4201)
		union {
			struct {
				PrimitiveType::Enum type;
				texture::TexID textureId;
			};
			uint64_t flags;
		};
		X_ENABLE_WARNING(4201)

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