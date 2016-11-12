#pragma once

#include "IPrimativeContext.h"

X_NAMESPACE_BEGIN(engine)

class PrimativeContext : public IPrimativeContext
{
public:

	struct PrimFlagBitMasks
	{
		enum Enum : uint32_t
		{
			TextureIdShift = 0,
			TextureIdBits = 20,
			TextureIdMask = ((1 << TextureIdBits) - 1) << TextureIdShift,

			PrimTypeShift = TextureIdBits,
			PrimTypeBits = 3,
			PrimTypeMask = ((1 << PrimTypeBits) - 1) << PrimTypeShift,
		};
	};

	static_assert(PrimitiveType::ENUM_COUNT < 7, "Primt enum count don't fit in prim type mask");

	struct PrimRenderFlags
	{
		X_INLINE PrimRenderFlags(PrimitiveType::Enum type, texture::TexID textureId);

		X_INLINE bool operator ==(const PrimRenderFlags& rhs) const;
		X_INLINE bool operator !=(const PrimRenderFlags& rhs) const;
		X_INLINE bool operator <(const PrimRenderFlags& rhs) const;

		X_INLINE PrimitiveType::Enum getPrimType(void) const;
		X_INLINE texture::TexID getTextureId(void) const;

		X_INLINE uint32_t toInt(void) const;

	private:
		uint32_t flags_;
	};

	struct PushBufferEntry
	{
		PushBufferEntry() = default;

		X_INLINE PushBufferEntry(uint32 numVertices, uint32 vertexOffs,
			PrimRenderFlags flags, render::StateHandle stateHandle);


		uint32 numVertices;
		uint32 vertexOffs;
		PrimRenderFlags flags;

		render::StateHandle stateHandle;
	};


	typedef core::Array<PushBufferEntry> PushBufferArr;
	typedef core::Array<const PushBufferEntry*> SortedPushBufferArr;
	typedef core::Array<PrimVertex, core::ArrayAlignedAllocator<PrimVertex>> VertexArr;

	X_ENSURE_SIZE(PushBufferEntry, 16); // not important, just ensuring padd correct.

private:

	static const uint32_t NUMVERTS_PER_PAGE = 1024 * 32;


public:
	PrimativeContext(core::MemoryArenaBase* arena);
	~PrimativeContext() X_OVERRIDE;

	bool createStates(render::IRender* pRender);
	bool freeStates(render::IRender* pRender);

	void reset(void) X_FINAL;
	
	bool isEmpty(void) const;
	void getSortedBuffer(SortedPushBufferArr& sortedPushBuffer) const;
	const PushBufferArr& getUnsortedBuffer(void) const;
	const VertexArr& getVerts(void) const;
	const render::VertexBufferHandle getVertBufHandle(void) const;

	void drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pBegin, const char* pEnd) X_FINAL;

private:
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type, texture::TexID textureId, render::StateHandle stateHandle) X_FINAL;
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type, texture::TexID textureId) X_FINAL;
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type) X_FINAL;

private:
	PushBufferArr pushBufferArr_;
	VertexArr vertexArr_;

	render::PassStateHandle passHandle_;
	render::StateHandle stateCache_[PrimitiveType::ENUM_COUNT];
	render::StateHandle textDrawState_;

	render::shader::IShader* pAuxShader_;
	render::shader::IShader* pTextShader_;

	render::VertexBufferHandle vertexBuf_;
};



X_NAMESPACE_END

#include "PrimativeContext.inl"