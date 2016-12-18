#pragma once

#include "EngineBase.h"

#include <IRenderCommands.h>

#include "IPrimativeContext.h"

X_NAMESPACE_BEGIN(engine)


class PrimativeContext : public IPrimativeContext, public XEngineBase
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
			Material* pMaterial);

		// 4
		uint16_t numVertices;
		uint16_t vertexOffs;
		// 4
		int32_t pageIdx; // need to try find a good place to put this, 3 bits is enougth to store this.

		// 4
	//	PrimRenderFlags flags; // we need these if we have material :| ?
	//	// 4
	//	uint32_t _pad;

		// 8
	//	render::StateHandle stateHandle;
		// material.
		// int32_t material;
		Material* pMaterial;
	};

#if X_64
	X_ENSURE_SIZE(PushBufferEntry, 16); // not important, just ensuring padd correct.
#else
	X_ENSURE_SIZE(PushBufferEntry, 12); 
#endif

	typedef core::Array<PushBufferEntry> PushBufferArr;
	typedef core::Array<const PushBufferEntry*> SortedPushBufferArr;
	typedef core::Array<PrimVertex, core::ArrayAlignedAllocator<PrimVertex>> VertexArr;

	struct VertexPage
	{
		VertexPage(core::MemoryArenaBase* arena);

		X_INLINE void reset(void);

		void createVB(render::IRender* pRender);
		void destoryVB(render::IRender* pRender);

		X_INLINE bool isVbValid(void) const;

		X_INLINE const uint32_t getVertBufBytes(void) const;
		X_INLINE const uint32_t freeSpace(void) const;

	public:
		render::VertexBufferHandle vertexBufHandle;
		VertexArr verts;
	};

	typedef core::Array<VertexPage> VertexPagesArr;


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

	static_assert(NUMVERTS_PER_PAGE < std::numeric_limits<decltype(PushBufferEntry::vertexOffs)>::max(),
		"Verts per page exceeds numerical limit of offset type");
	static_assert(NUMVERTS_PER_PAGE < std::numeric_limits<decltype(PushBufferEntry::numVertices)>::max(),
		"Verts per page exceeds numerical limit of count type");

	typedef std::array<render::VertexBufferHandle, MAX_PAGES> VertexPageHandlesArr;


public:
	PrimativeContext(core::MemoryArenaBase* arena);
	~PrimativeContext() X_OVERRIDE;

	bool createStates(render::IRender* pRender);
	bool freeStates(render::IRender* pRender);

	void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const;

	void reset(void) X_FINAL;
	
	bool isEmpty(void) const;
	const PushBufferArr& getUnsortedBuffer(void) const;
	VertexPageHandlesArr getVertBufHandles(void) const;

	void drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pBegin, const char* pEnd) X_FINAL;
	void drawText(const Vec3f& pos, const font::TextDrawContext& con, const wchar_t* pBegin, const wchar_t* pEnd) X_FINAL;

private:
	VertexPage& getPage(size_t requiredVerts);

private:
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type, Material* pMaterial) X_FINAL;
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type) X_FINAL;

private:
	render::IRender* pRender_; // we lazy create vertexBuffers for pages.

	PushBufferArr pushBufferArr_;
	VertexPagesArr vertexPages_;

	int32_t currentPage_;

	render::PassStateHandle passHandle_;
	render::StateHandle stateCache_[PrimitiveType::ENUM_COUNT];

	Material* primMaterials_[PrimitiveType::ENUM_COUNT];
};



X_NAMESPACE_END

#include "PrimativeContext.inl"
