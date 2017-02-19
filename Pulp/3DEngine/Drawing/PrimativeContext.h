#pragma once

#include "EngineBase.h"

#include <IRenderCommands.h>
#include "IPrimativeContext.h"

#include <Util\PointerFlags.h>

X_NAMESPACE_BEGIN(engine)


X_DISABLE_WARNING(4324) //  structure was padded due to alignment specifier

// this stores resources that are shared between contex's.
// like materials and shape meshes.
X_ALIGNED_SYMBOL(class PrimativeContextSharedResources, 64) : public XEngineBase
{
	static const int32_t SHAPES_NUM_LOD = IPrimativeContext::SHAPE_NUM_LOD;

	typedef IPrimativeContext::PrimitiveType PrimitiveType;
	typedef IPrimativeContext::ShapeInstanceData ShapeInstanceData;
	typedef IPrimativeContext::ShapeType ShapeType;

	typedef Vertex_P3F_T2S_C4B ShapeVertex;

	typedef core::Array<ShapeVertex, core::ArrayAlignedAllocator<ShapeVertex>> VertArr;
	typedef core::Array<uint16_t, core::ArrayAlignedAllocator<uint16_t>> IndexArr;

public:
	struct ShapeLod
	{
		ShapeLod();

		uint16_t indexCount;
		uint16_t startIndex;
		uint16_t baseVertex;
	};

	typedef std::array<ShapeLod, SHAPES_NUM_LOD> ShapeLodArr;

	// we put all the lods for a given shape in a single buffer.
	// that way we don't need to switch when rendering same shape with diffrent active lods.
	// potentialy doing something else might work out better in pratice, like all shapes of given lod index in same buffer etc..
	struct Shape
	{
		Shape();

		ShapeLodArr lods;
		render::VertexBufferHandle vertexBuf;
		render::IndexBufferHandle indexbuf;
	};

	typedef std::array<Material*, PrimitiveType::ENUM_COUNT> PrimMaterialArr;
	typedef std::array<Shape, ShapeType::ENUM_COUNT> PrimShapeArr;


	// pages for instanced data.
	static const uint32_t NUM_INSTANCE_PER_PAGE = 0x100;
	static const uint32_t INSTANCE_PAGE_BYTES = NUM_INSTANCE_PER_PAGE * sizeof(ShapeInstanceData);
	static const uint32_t MAX_PAGES = 16;
	static const uint32_t MAX_INSTANCE_TOTAL = NUM_INSTANCE_PER_PAGE * MAX_PAGES;

	struct InstancedPage
	{
		InstancedPage();

		void createVB(render::IRender* pRender);
		void destoryVB(render::IRender* pRender);

		X_INLINE void reset(void);

		X_INLINE bool isVbValid(void) const;

		X_INLINE uint32_t curOffset(void) const;
		X_INLINE uint32_t curOffsetBytes(void) const;
		X_INLINE size_t spaceLeft(void) const;
		X_INLINE size_t spaceLeftBytes(void) const;
		X_INLINE void incOffset(uint32_t num);

	public:
		uint32_t currentOffset;
		render::VertexBufferHandle instBufHandle;
	};

	typedef core::FixedArray<InstancedPage, MAX_PAGES> InstancedPageArr;

public:
	PrimativeContextSharedResources();

	bool init(render::IRender* pRender);
	void releaseResources(render::IRender* pRender);

	X_INLINE Material* getMaterial(PrimitiveType::Enum prim) const;
	X_INLINE Material* getMaterialDepthTest(PrimitiveType::Enum prim) const;

	X_INLINE const Shape& getShapeResources(ShapeType::Enum shape) const;

	void resetInstancePages(void);
	bool getFreeInstancePage(InstancedPage& out);

private:
	bool loadMaterials(void);
	bool createShapeBuffers(render::IRender* pRender);

private:
	static void CreateSphere(VertArr& vb, IndexArr& ib,
		float radius, uint32_t rings, uint32_t sections);

	static void CreateCone(VertArr& vb, IndexArr& ib,
		float radius, float height, uint32_t sections);

	static void CreateCylinder(VertArr& vb, IndexArr& ib,
		float radius, float height, uint32_t sections);

private:
	PrimMaterialArr primMaterials_;
	PrimMaterialArr primMaterialsDepth_;
	PrimShapeArr shapes_;

	int32_t currentInstacePageIdx_;
	InstancedPageArr shapeInstancePages_;
};


X_ALIGNED_SYMBOL(class PrimativeContext, 64) : public IPrimativeContext, public XEngineBase
{
public:
	typedef core::PointerFlags<Material, 3> MaterialWithPageIdx;

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
	//	int32_t pageIdx; // need to try find a good place to put this, 3 bits is enougth to store this.


	// if i fource all materials to be 8byte aligned i can store the page index in here :D
	// could props get away with shoving it in the msb's but not as portable and safe.
		MaterialWithPageIdx material;
	//	Material* pMaterial;
	};

#if X_64
	X_ENSURE_SIZE(PushBufferEntry, 16); // not important, just ensuring padd correct.
#else
	X_ENSURE_SIZE(PushBufferEntry, 8); 
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
	typedef core::Array<ShapeInstanceData, core::ArrayAlignedAllocator<ShapeInstanceData>> ShapeParamArr;

	// we have diffrent arrays for each lod of each shape.
	// just makes submission more simple.
	typedef std::array<ShapeParamArr, SHAPE_NUM_LOD> ShapeTypeLodArr; // come up with better name for this?
	typedef std::array<ShapeTypeLodArr, ShapeType::ENUM_COUNT> ShapeParamLodTypeArr; // come up with better name for this?

private:

	// I think i'm going to just support pages of verts.
	// so we just allocate more pages if we run out of space.
	// this way we can make vertexoffset and numVertex 16bit.
	// and just store the page index.
	// this will also lets use do a form of GC as we can free pages that have not been used in a while.
	// allowing us to support drawing large amounts but claming back the memory after it's not used.
	static const uint32_t NUMVERTS_PER_PAGE = 0xaaa * 16;
	static const uint32_t PAGE_BYTES = NUMVERTS_PER_PAGE * sizeof(PrimVertex);
	static const uint32_t MAX_PAGES = MaterialWithPageIdx::BIT_MASK + 1; // what ever we can fit in the bits is the max.
	static const uint32_t MAX_VERTS_TOTAL = NUMVERTS_PER_PAGE * MAX_PAGES;

	static_assert(NUMVERTS_PER_PAGE < std::numeric_limits<decltype(PushBufferEntry::vertexOffs)>::max(),
		"Verts per page exceeds numerical limit of offset type");
	static_assert(NUMVERTS_PER_PAGE < std::numeric_limits<decltype(PushBufferEntry::numVertices)>::max(),
		"Verts per page exceeds numerical limit of count type");

	typedef std::array<render::VertexBufferHandle, MAX_PAGES> VertexPageHandlesArr;


public:
	PrimativeContext(PrimativeContextSharedResources& sharedRes, Mode mode, core::MemoryArenaBase* arena);
	~PrimativeContext() X_OVERRIDE;

	bool freePages(render::IRender* pRender);

	void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const;

	size_t maxVertsPerPrim(void) const X_FINAL;
	Mode getMode(void) const X_FINAL;
	void reset(void) X_FINAL;
	void setDepthTest(bool enabled) X_FINAL;

	bool isEmpty(void) const;
	bool hasShapeData(void) const;
	const PushBufferArr& getUnsortedBuffer(void) const;
	const ShapeParamLodTypeArr& getShapeArrayBuffers(void) const;
	VertexPageHandlesArr getVertBufHandles(void) const;
	const PrimativeContextSharedResources::Shape& getShapeResources(ShapeType::Enum shape) const;

public:
	void drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pBegin, const char* pEnd) X_FINAL;
	void drawText(const Vec3f& pos, const font::TextDrawContext& con, const wchar_t* pBegin, const wchar_t* pEnd) X_FINAL;

private:
	VertexPage& getPage(size_t requiredVerts);

private:
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type, Material* pMaterial) X_FINAL;
	PrimVertex* addPrimative(uint32_t num, PrimitiveType::Enum type) X_FINAL;
	ShapeInstanceData* addShape(ShapeType::Enum type, int32_t lodIdx = 0) X_FINAL;

private:
	PushBufferArr pushBufferArr_;
	VertexPagesArr vertexPages_;

	int32_t depthPrim_;
	int32_t currentPage_;
	Mode mode_;

	ShapeParamLodTypeArr shapeLodArrays_;

	const PrimativeContextSharedResources& sharedRes_;
};



X_NAMESPACE_END

#include "PrimativeContext.inl"


X_ENABLE_WARNING(4324) //  structure was padded due to alignment specifier
