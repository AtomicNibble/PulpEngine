#pragma once

#include "EngineBase.h"

#include <IRenderCommands.h>

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

	// should i make this POD so growing the push buffer is faster. humm.
	struct PushBufferEntry
	{
		PushBufferEntry() = default;

		X_INLINE PushBufferEntry(uint16 numVertices, uint16 vertexOffs, int32_t pageIdx,
			PrimRenderFlags flags, render::StateHandle stateHandle);

		// 4
		uint16_t numVertices;
		uint16_t vertexOffs;
		// 4
		int32_t pageIdx;
		// 4
		PrimRenderFlags flags; // we need these if we have material :| ?
		uint32_t _pad;

		// 8
		render::StateHandle stateHandle;
		// material.
		// int32_t material;
	};


	typedef core::Array<PushBufferEntry> PushBufferArr;
	typedef core::Array<const PushBufferEntry*> SortedPushBufferArr;
	typedef core::Array<PrimVertex, core::ArrayAlignedAllocator<PrimVertex>> VertexArr;

	struct VertexPage
	{
		VertexPage(core::MemoryArenaBase* arena);

		void reset(void);

		void createVB(render::IRender* pRender);
		void destoryVB(render::IRender* pRender);

		bool isVbValid(void) const;

		const uint32_t getVertBufBytes(void) const;
		const uint32_t freeSpace(void) const;

	public:
		render::VertexBufferHandle vertexBufHandle;
		VertexArr verts;
	};

	typedef core::Array<VertexPage> VertexPagesArr;

	X_ENSURE_SIZE(PushBufferEntry, 24); // not important, just ensuring padd correct.

private:

	// I think i'm going to just support pages of verts.
	// so we just allocate more pages if we run out of space.
	// this way we can make vertexoffset and numVertex 16bit.
	// and just store the page index.
	// this will also lets use do a form of GC as we can free pages that have not been used in a while.
	// allowing us to support drawing large amounts but claming back the memory after it's not used.
	static const uint32_t NUMVERTS_PER_PAGE = 0xaaa * 16;
	static const uint32_t PAGE_BYTES = NUMVERTS_PER_PAGE * sizeof(PrimVertex);
	static const uint32_t MAX_PAGES = 8; // lets not go mental.

	typedef std::array<render::VertexBufferHandle, MAX_PAGES> VertexPageHandlesArr;


public:
	PrimativeContext(core::MemoryArenaBase* arena);
	~PrimativeContext() X_OVERRIDE;

	bool createStates(render::IRender* pRender);
	bool freeStates(render::IRender* pRender);

	void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const;

	void reset(void) X_FINAL;
	
	bool isEmpty(void) const;
	void getSortedBuffer(SortedPushBufferArr& sortedPushBuffer) const;
	const PushBufferArr& getUnsortedBuffer(void) const;
	VertexPageHandlesArr getVertBufHandles(void) const;

	void drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pBegin, const char* pEnd) X_FINAL;
	void drawText(const Vec3f& pos, const font::TextDrawContext& con, const wchar_t* pBegin, const wchar_t* pEnd) X_FINAL;

private:
	VertexPage& getPage(size_t requiredVerts);

private:
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type, texture::TexID textureId, render::StateHandle stateHandle) X_FINAL;
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type, texture::TexID textureId) X_FINAL;
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type) X_FINAL;

private:
	render::IRender* pRender_; // we lazy create vertexBuffers for pages.

	PushBufferArr pushBufferArr_;
	VertexPagesArr vertexPages_;

	int32_t currentPage_;

	render::PassStateHandle passHandle_;
	render::StateHandle stateCache_[PrimitiveType::ENUM_COUNT];
	render::StateHandle textDrawState_;

	render::shader::IShader* pAuxShader_;
	render::shader::IShader* pTextShader_;

};



X_NAMESPACE_END

#include "PrimativeContext.inl"
