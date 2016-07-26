#pragma once

#include "AuxRender.h"
#include <Math\XPlane.h>

X_NAMESPACE_BEGIN(render)

struct AuxGeomCBRawDataPackaged
{
	X_INLINE AuxGeomCBRawDataPackaged(const RenderAux::AuxGeomCBRawData* pData);

	const RenderAux::AuxGeomCBRawData* pData_;
};


struct AuxObjVertex
{
	X_INLINE AuxObjVertex() = default;
	X_INLINE AuxObjVertex(const Vec3f& pos, const Vec3f& normal);

	Vec3f pos;
	Vec3f normal;
};



struct AuxObjMesh
{
	X_INLINE AuxObjMesh();
	X_INLINE ~AuxObjMesh();

	void release(void);

	uint32 numVertices;
	uint32 numFaces;
	uint32 VBid;
	uint32 IBid;
};


// this should move somewhere more common
struct PrimitiveType
{
	enum Enum
	{
		TriangleList = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		TriangleStrip = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
		LineList = D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
		LineStrip = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
		PointList = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST
	};
};


class RenderAuxImp : public IRenderAuxImpl
{
	static const int32_t AUX_OBJ_NUM_LOD = 5;
	static const int32_t AUX_GEOM_VBSIZE;
	static const int32_t AUX_GEOM_IBSIZE;
	static const float32_t CLIP_THRESHOLD;

public:
	RenderAuxImp(core::MemoryArenaBase* arena);
	~RenderAuxImp() X_OVERRIDE;

	bool init(void);
	void shutDown(void);

	// IRenderAuxImpl
	virtual void flush(const AuxGeomCBRawDataPackaged& data, size_t begin, size_t end) X_OVERRIDE;
	// ~IRenderAuxImpl


private:
	X_INLINE const Matrix44f& getCurrentView(void) const;
	X_INLINE const Matrix44f& getCurrentViewInv(void) const;
	X_INLINE const Matrix44f& getCurrentProj(void) const;
	X_INLINE const Matrix44f& getCurrentTrans3D(void) const;
	X_INLINE const Matrix44f& getCurrentTrans2D(void) const;

	X_INLINE const RenderAux::AuxVertexArr& getAuxVertexBuffer(void) const;
	X_INLINE const RenderAux::AuxIndexArr& getAuxIndexBuffer(void) const;
	X_INLINE const RenderAux::AuxDrawObjParamArr& getAuxDrawObjParamBuffer(void) const;
	X_INLINE const Matrix44f& getAuxOrthoMatrix(int32_t idx) const;
	X_INLINE bool isOrthoMode(void) const;



	void determineAuxPrimitveFlags(uint32& d3dNumPrimDivider, PrimitiveType::Enum& d3dPrim, RenderAux::PrimType::Enum primType) const;
	void drawAuxPrimitives(RenderAux::AuxSortedPushArr::ConstIterator itBegin, RenderAux::AuxSortedPushArr::ConstIterator itEnd, 
		const RenderAux::PrimType::Enum primType);
	void drawAuxIndexedPrimitives(RenderAux::AuxSortedPushArr::ConstIterator itBegin, RenderAux::AuxSortedPushArr::ConstIterator itEnd, 
		const RenderAux::PrimType::Enum primType);
	void drawAuxObjects(RenderAux::AuxSortedPushArr::ConstIterator itBegin, RenderAux::AuxSortedPushArr::ConstIterator itEnd);

	void prepareThickLines2D(RenderAux::AuxSortedPushArr::ConstIterator itBegin, RenderAux::AuxSortedPushArr::ConstIterator itEnd);
	void prepareThickLines3D(RenderAux::AuxSortedPushArr::ConstIterator itBegin, RenderAux::AuxSortedPushArr::ConstIterator itEnd);

private:
	bool createLods(core::MemoryArenaBase* arena);
	void releaseLods(void);

	template< typename TMeshFunc >
	bool createMesh(core::MemoryArenaBase* arena, AuxObjMesh& mesh, TMeshFunc meshFunc);


private:
	bool clipLine(Vec3f v[2], Color8u c[2]);

	static X_INLINE Color8u clipColor(const Color8u& c0, const Color8u& c1, float32_t t);
	static X_INLINE Vec3f intersectLinePlane(const Vec3f& o, const Vec3f& d, const Planef& p, float32_t& t);
	static float32_t computeConstantScale(const Vec3f& v, const Matrix44f& matView,
		const Matrix44f& matProj, const uint32 wndXRes);

private:

	struct Matrices
	{
		Matrix44f matView;
		Matrix44f matViewInv;
		Matrix44f matProj;
		Matrix44f matTrans3D;
		Matrix44f matTrans2D;
	};

private:
	core::MemoryArenaBase* arena_;

	RenderAux::PrimType::Enum curPrimType_;
	AuxGeom_DrawInFrontMode::Enum curDrawInFrontMode_;
	const RenderAux::AuxGeomCBRawData* pCurCBRawData_;

	RenderAux::AuxSortedPushArr auxSortedPushArr_;

	uint32 wndXRes_;
	uint32 wndYRes_;
	float32_t aspect_;
	float32_t aspectInv_;
	Planef nearPlane_; // used for clipping lines

	Matrices matrices_;

	AuxObjMesh sphereObj_[AUX_OBJ_NUM_LOD];
	AuxObjMesh coneObj_[AUX_OBJ_NUM_LOD];
	AuxObjMesh cylinderObj_[AUX_OBJ_NUM_LOD];
};


X_NAMESPACE_END

#include "AuxRenderImp.inl"