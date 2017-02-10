#pragma once


#include <IRenderAux.h>
#include <Math\VertexFormats.h>


X_NAMESPACE_BEGIN(render)

struct AuxGeomCBRawDataPackaged;

struct IRenderAuxImpl
{
public:
	virtual ~IRenderAuxImpl() {}
	virtual void flush(const AuxGeomCBRawDataPackaged& data, size_t begin, size_t end) X_ABSTRACT;
};

struct AuxDrawObjParams
{
	AuxDrawObjParams() = default;
	X_INLINE AuxDrawObjParams(const Matrix34f& matWorld, const Color8u& color,
		float32_t size, bool shaded);

	Matrix34f matWorld;
	Color8u color;
	float32_t size;
	bool shaded;
	bool _pad[4];
};

class RenderAuxImp;
class RenderAux : public IRenderAux
{
	friend class RenderAuxImp;
	friend struct AuxGeomCBRawDataPackaged;

	X_DECLARE_ENUM(DrawObjType)(Sphere, Cone, Cylinder);
	X_DECLARE_ENUM(PrimType)(Invalid, PointList, LineList, LineListInd, TriList, TriListInd, Obj);

	struct AuxGeomPrivateBitMasks
	{
		// public field starts at bit 22
		enum Enum
		{
			PrimTypeShift = 19,
			PrimTypeMask = 0x7 << PrimTypeShift,
			PrivateRenderflagsMask = (1 << 19) - 1
		};
	};

	struct AuxGeomPrivateRenderflags
	{
		// for non-indexed triangles
		enum Enum
		{
			TriListParam_ProcessThickLines = 0x00000001,
		};
	};

	struct AuxPushBufferEntry
	{
		AuxPushBufferEntry() = default;
		X_INLINE AuxPushBufferEntry(uint32 numVertices, uint32 numIndices,
			uint32 vertexOffs, uint32 indexOffs,
			const XAuxGeomRenderFlags& renderFlags);

		X_INLINE AuxPushBufferEntry(uint32 drawParamOffs,
			const XAuxGeomRenderFlags& renderFlags);

		X_INLINE bool getDrawParamOffs(uint32& drawParamOffs) const;


		uint32 numVertices;
		uint32 numIndices;
		uint32 vertexOffs;
		uint32 indexOffs;
		int32_t transMatrixIdx;
		XAuxGeomRenderFlags renderFlags;
	};


	typedef core::Array<AuxPushBufferEntry> AuxPushArr;
	typedef core::Array<const AuxPushBufferEntry*> AuxSortedPushArr;
	typedef core::Array<XAuxVertex> AuxVertexArr;
	typedef core::Array<uint16_t> AuxIndexArr;
	typedef core::Array<AuxDrawObjParams> AuxDrawObjParamArr;
	typedef core::Array<Matrix44f> AuxOrthoMatrixArr;

protected:
	struct AuxGeomCBRawData
	{
	public:
		AuxGeomCBRawData(core::MemoryArenaBase* arena);

		void getSortedPushBuffer(size_t begin, size_t end,
			AuxSortedPushArr& auxSortedPushBuffer) const;

		void reset(void);
		void free(void);

	public:
		AuxPushArr auxPushArr;
		AuxVertexArr auxVertexArr;
		AuxIndexArr auxIndexArr;
		AuxDrawObjParamArr auxDrawObjParamArr;
		AuxOrthoMatrixArr auxOrthoMatrices;
	};

public:
	RenderAux(core::MemoryArenaBase* arena);
	~RenderAux() X_OVERRIDE;

	void flush(void) X_OVERRIDE;
	void clear(void) X_OVERRIDE;

	X_INLINE virtual void setRenderFlags(const XAuxGeomRenderFlags& renderFlags) X_OVERRIDE;
	X_INLINE virtual XAuxGeomRenderFlags getRenderFlags(void) X_OVERRIDE;

	// Lines
	void drawLine(const Vec3f& v0, const Color8u& c0, const Vec3f& v1, const Color8u& c1, float thickness = 1.f) X_OVERRIDE;

	void drawLines(Vec3f* pPoints, uint32_t numPoints, const Color8u& col, float thickness = 1.f) X_OVERRIDE;
	void drawLines(Vec3f* pPoints, uint32_t numPoints, Color8u* col, float thickness = 1.f) X_OVERRIDE;

	void drawLines(Vec3f* pPoints, uint32_t numPoints, uint16_t* pIndices,
		uint32_t numIndices, const Color8u& col, float thickness = 1.f) X_OVERRIDE;
	void drawLines(Vec3f* pPoints, uint32_t numPoints, uint16_t* pIndices,
		uint32_t numIndices, Color8u* pCol, float thickness = 1.f) X_OVERRIDE;

	// Triangles
	void drawTriangle(const Vec3f& v0, const Color8u& c0, const Vec3f& v1, const Color8u& c1,
		const Vec3f& v2, const Color8u& c2) X_OVERRIDE;
	virtual void drawTriangle(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, const Color8u& col) X_OVERRIDE;

	void drawTriangle(const Vec3f* pPoints, uint32_t numPoints, const Color8u& c0) X_OVERRIDE;
	void drawTriangle(const Vec3f* pPoints, uint32_t numPoints, const Color8u* pCol) X_OVERRIDE;

	void drawTriangle(const Vec3f* pPoints, uint32_t numPoints, const uint16_t* pIndices,
		uint32_t numIndices, const Color8u& c0) X_OVERRIDE;
	void drawTriangle(const Vec3f* pPoints, uint32_t numPoints, const uint16_t* pIndices,
		uint32_t numIndices, const Color8u* pCol) X_OVERRIDE;

	// Point
	void drawPoint(const Vec3f &pos, const Color8u& col, uint8_t size = 1) X_OVERRIDE;
	void drawPoints(Vec3f* pPoints, uint32_t numPoints, const Color8u& col, uint8_t size = 1) X_OVERRIDE;
	void drawPoints(Vec3f* pPoints, uint32_t numPoints, Color8u* pCol, uint8_t size = 1) X_OVERRIDE;

	// AABB
	void drawAABB(const AABB& aabb, bool solid, const Color8u& col) X_OVERRIDE;
	void drawAABB(const AABB& aabb, const Vec3f& pos, bool solid, const Color8u& col) X_OVERRIDE;
	void drawAABB(const AABB& aabb, const Matrix34f& mat, bool solid, const Color8u& col) X_OVERRIDE;

	// OBB
	void drawOBB(const OBB& obb, const Vec3f& pos, bool solid, const Color8u& col) X_OVERRIDE;
	void drawOBB(const OBB& obb, const Matrix34f& mat, bool solid, const Color8u& col) X_OVERRIDE;

	// Sphere
	void drawSphere(const Sphere& sphere, const Color8u& col, bool drawShaded = true) X_OVERRIDE;
	void drawSphere(const Sphere& sphere, const Matrix34f& mat, const Color8u& col, bool drawShaded = true) X_OVERRIDE;

	// Cone
	void drawCone(const Vec3f& pos, const Vec3f& dir, float radius, float height, const Color8u& col, bool drawShaded = true) X_OVERRIDE;

	// Cylinder
	void drawCylinder(const Vec3f& pos, const Vec3f& dir, float radius, float height, const Color8u& col, bool drawShaded = true) X_OVERRIDE;

	// Bone
	void drawBone(const Transformf& rParent, const Transformf& rBone, const Color8u& col) X_OVERRIDE;
	void drawBone(const Matrix34f& rParent, const Matrix34f& rBone, const Color8u& col) X_OVERRIDE;

	// Frustum
	void drawFrustum(const XFrustum& frustum, const Color8u& nearCol, const Color8u& farCol, bool drawShaded = false) X_OVERRIDE;

	// Arrow
	void drawArrow(const Vec3f& posA, const Vec3f& posB, const Color8u& color) X_OVERRIDE;


private:
	static X_INLINE PrimType::Enum getPrimType(const XAuxGeomRenderFlags& renderFlags);
	static X_INLINE bool isThickLine(const XAuxGeomRenderFlags& renderFlags);
	static X_INLINE DrawObjType::Enum getAuxObjType(const XAuxGeomRenderFlags& renderFlags);
	static X_INLINE uint8 getPointSize(const XAuxGeomRenderFlags& renderFlags);


private:
	X_INLINE uint32 createPointRenderFlags(uint8 size);
	X_INLINE uint32 createLineRenderFlags(bool indexed);
	X_INLINE uint32 createTriangleRenderFlags(bool indexed);
	X_INLINE uint32 createObjectRenderFlags(const DrawObjType::Enum objType);

	void drawThickLine(const Vec3f& v0, const Color8u& col0,
		const Vec3f& v1, const Color8u& col1, float thickness);

	void addPushBufferEntry(uint32 numVertices, uint32 numIndices,
		const XAuxGeomRenderFlags& renderFlags);

	void addPrimitive(XAuxVertex*& pVertices, uint32 numVertices,
		const XAuxGeomRenderFlags& renderFlags);
	void addIndexedPrimitive(XAuxVertex*& pVertices, uint32 numVertices,
		uint16*& pIndices, uint32 numIndices, const XAuxGeomRenderFlags& renderFlags);

	void addObject(AuxDrawObjParams*& pDrawParams, const XAuxGeomRenderFlags& renderFlags);


private:
	XAuxGeomRenderFlags curRenderFlags_;
	AuxGeomCBRawData data_;
};



X_NAMESPACE_END

#include "AuxRender.inl"