#pragma once


#ifndef X_RENDER_AUG_I_H_
#define X_RENDER_AUG_I_H_

class AABB;
class OBB;
class Sphere;

X_NAMESPACE_BEGIN(render)

struct XAuxGeomRenderFlags;

struct AuxGeomBitMasks
{
	enum Enum : uint32_t
	{
		Mode2D3DShift = 31,
		Mode2D3DMask = 0x1u << Mode2D3DShift,

		AlphaBlendingShift = 29,
		AlphaBlendingMask = 0x3 << AlphaBlendingShift,

		DrawInFrontShift = 28,
		DrawInFrontMask = 0x1 << DrawInFrontShift,

		FillModeShift = 26,
		FillModeMask = 0x3 << FillModeShift,

		CullModeShift = 24,
		CullModeMask = 0x3 << CullModeShift,

		DepthWriteShift = 23,
		DepthWriteMask = 0x1 << DepthWriteShift,

		DepthTestShift = 22,
		DepthTestMask = 0x1 << DepthTestShift,

		PublicParamsMask = Mode2D3DMask | AlphaBlendingMask | DrawInFrontMask | FillModeMask |
		CullModeMask | DepthWriteMask | DepthTestMask
	};
};


struct AuxGeom_Mode2D3D
{
	enum Enum : uint32_t
	{
		Mode3D = 0x0 << AuxGeomBitMasks::Mode2D3DShift,
		Mode2D = 0x1u << AuxGeomBitMasks::Mode2D3DShift,
	};
};

struct AuxGeom_AlphaBlendMode
{
	enum Enum : uint32_t
	{

		AlphaNone = 0x0 << AuxGeomBitMasks::AlphaBlendingShift,
		AlphaAdditive = 0x1 << AuxGeomBitMasks::AlphaBlendingShift,
		AlphaBlended = 0x2 << AuxGeomBitMasks::AlphaBlendingShift,
	};
};

struct AuxGeom_DrawInFrontMode
{
	enum Enum : uint32_t
	{
		DrawInFrontOff = 0x0 << AuxGeomBitMasks::DrawInFrontShift,
		DrawInFrontOn = 0x1 << AuxGeomBitMasks::DrawInFrontShift,
	};
};

struct AuxGeom_FillMode
{
	enum Enum : uint32_t
	{
		FillModeSolid = 0x0 << AuxGeomBitMasks::FillModeShift,
		FillModeWireframe = 0x1 << AuxGeomBitMasks::FillModeShift,
		FillModePoint = 0x2 << AuxGeomBitMasks::FillModeShift,
	};
};

struct AuxGeom_CullMode
{
	enum Enum : uint32_t
	{
		CullModeNone = 0x0 << AuxGeomBitMasks::CullModeShift,
		CullModeFront = 0x1 << AuxGeomBitMasks::CullModeShift,
		CullModeBack = 0x2 << AuxGeomBitMasks::CullModeShift,
	};
};

struct AuxGeom_DepthWrite
{
	enum Enum : uint32_t
	{
		DepthWriteOn = 0x0 << AuxGeomBitMasks::DepthWriteShift,
		DepthWriteOff = 0x1 << AuxGeomBitMasks::DepthWriteShift,
	};
};


struct AuxGeom_DepthTest
{
	enum Enum : uint32_t
	{
		DepthTestOn = 0x0 << AuxGeomBitMasks::DepthTestShift,
		DepthTestOff = 0x1 << AuxGeomBitMasks::DepthTestShift,
	};
};


struct AuxGeom_Defaults
{
	enum Enum : uint32_t
	{
		// Default render flags for 3d primitives.
		Def3DRenderflags = AuxGeom_Mode2D3D::Mode3D |
		AuxGeom_AlphaBlendMode::AlphaNone | AuxGeom_DrawInFrontMode::DrawInFrontOff |
		AuxGeom_FillMode::FillModeSolid | AuxGeom_CullMode::CullModeBack |
		AuxGeom_DepthWrite::DepthWriteOn | AuxGeom_DepthTest::DepthTestOn,

		// Default render flags for 2d primitives.
		Def2DPRenderflags = AuxGeom_Mode2D3D::Mode2D | 
		AuxGeom_AlphaBlendMode::AlphaNone | AuxGeom_DrawInFrontMode::DrawInFrontOff | 
		AuxGeom_FillMode::FillModeSolid | AuxGeom_CullMode::CullModeBack |
		AuxGeom_DepthWrite::DepthWriteOn | AuxGeom_DepthTest::DepthTestOn
	};

};




struct IRenderAux
{
	virtual ~IRenderAux() {}

	virtual void flush(void) X_ABSTRACT;
	virtual void clear(void) X_ABSTRACT;

	virtual void setRenderFlags(const XAuxGeomRenderFlags& renderFlags) X_ABSTRACT;
	virtual XAuxGeomRenderFlags getRenderFlags() X_ABSTRACT;

	// Lines
	virtual void drawLine(const Vec3f& v0, const Color8u& c0, const Vec3f& v1, const Color8u& c1, float thickness = 1.f) X_ABSTRACT;

	virtual void drawLines(Vec3f* points, uint32_t numPoints, const Color8u& col, float thickness = 1.f) X_ABSTRACT;
	virtual void drawLines(Vec3f* points, uint32_t numPoints, Color8u* pCol, float thickness = 1.f) X_ABSTRACT;

	virtual void drawLines(Vec3f* points, uint32_t numPoints, uint16_t* indices, uint32_t numIndices, const Color8u& col, float thickness = 1.f) X_ABSTRACT;
	virtual void drawLines(Vec3f* points, uint32_t numPoints, uint16_t* indices, uint32_t numIndices, Color8u* pCol, float thickness = 1.f) X_ABSTRACT;

	// Triangles
	virtual void drawTriangle(const Vec3f& v0, const Color8u& c0, const Vec3f& v1, const Color8u& c1, 
				const Vec3f& v2, const Color8u& c2 ) X_ABSTRACT;
	virtual void drawTriangle(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, const Color8u& col) X_ABSTRACT;

	virtual void drawTriangle(const Vec3f* points, uint32_t numPoints, const Color8u& c0) X_ABSTRACT;
	virtual void drawTriangle(const Vec3f* points, uint32_t numPoints, const Color8u* pCol) X_ABSTRACT;

	virtual void drawTriangle(const Vec3f* points, uint32_t numPoints, 
		const uint16_t* indices, uint32_t numIndices, const Color8u& c0) X_ABSTRACT;
	virtual void drawTriangle(const Vec3f* points, uint32_t numPoints, 
		const uint16_t* indices, uint32_t numIndices, const Color8u* pCol) X_ABSTRACT;

	// Point
	virtual void drawPoint(const Vec3f &pos, const Color8u& col, uint8_t size = 1) X_ABSTRACT;
	virtual void drawPoints(Vec3f* pPoints, uint32_t numPoints, const Color8u& col, uint8_t size = 1) X_ABSTRACT;
	virtual void drawPoints(Vec3f* pPoints, uint32_t numPoints, Color8u* pCol, uint8_t size = 1) X_ABSTRACT;


	// AABB
	virtual void drawAABB(const AABB& aabb, bool solid, const Color8u& col) X_ABSTRACT;
	virtual void drawAABB(const AABB& aabb, const Vec3f& pos, bool solid, const Color8u& col) X_ABSTRACT;
	virtual void drawAABB(const AABB& aabb, const Matrix34f& mat, bool solid, const Color8u& col) X_ABSTRACT;

	// OBB
	virtual void drawOBB(const OBB& obb, const Vec3f& pos, bool solid, const Color8u& col) X_ABSTRACT;
	virtual void drawOBB(const OBB& obb, const Matrix34f& mat, bool solid, const Color8u& col) X_ABSTRACT;

	// Sphere
	virtual void drawSphere(const Sphere& sphere, const Color8u& col, bool drawShaded = true) X_ABSTRACT;
	virtual void drawSphere(const Sphere& sphere, const Matrix34f& mat, const Color8u& col, bool drawShaded = true) X_ABSTRACT;

	// Cone
	virtual void drawCone(const Vec3f& pos, const Vec3f& dir, float radius, float height, const Color8u& col, bool drawShaded = true) X_ABSTRACT;

	// Cylinder
	virtual void drawCylinder(const Vec3f& pos, const Vec3f& dir, float radius, float height, const Color8u& col, bool drawShaded = true) X_ABSTRACT;

	// Bone
	virtual void drawBone(const Transformf& rParent, const Transformf& rBone, const Color8u& col) X_ABSTRACT;
	virtual void drawBone(const Matrix34f& rParent, const Matrix34f& rBone, const Color8u& col) X_ABSTRACT;

	// Frustum - Sexyyyyyyyyyy
	virtual void drawFrustum(const XFrustum& frustum, const Color8u& nearCol, const Color8u& farCol, bool drawShaded = false) X_ABSTRACT;

	// Arrow
	virtual void drawArrow(const Vec3f& posA, const Vec3f& posB, const Color8u& color) X_ABSTRACT;
};


struct XAuxGeomRenderFlags
{
public:
	X_INLINE XAuxGeomRenderFlags();
	X_INLINE XAuxGeomRenderFlags(const XAuxGeomRenderFlags& rhs);
	X_INLINE XAuxGeomRenderFlags(uint32 renderFlags);
	X_INLINE XAuxGeomRenderFlags& operator =(const XAuxGeomRenderFlags& rhs);
	X_INLINE XAuxGeomRenderFlags& operator =(uint32 rhs);

	X_INLINE bool operator ==(const XAuxGeomRenderFlags& rhs) const;
	X_INLINE bool operator ==(uint32 rhs) const;
	X_INLINE bool operator !=(const XAuxGeomRenderFlags& rhs) const;
	X_INLINE bool operator !=(uint32 rhs) const;

	X_INLINE operator uint32() {
		return renderFlags_;
	}

	X_INLINE operator const uint32() const {
		return renderFlags_;
	}

public:
	X_INLINE AuxGeom_Mode2D3D::Enum GetMode2D3DFlag() const;
	X_INLINE void SetMode2D3DFlag(const AuxGeom_Mode2D3D::Enum state);

	X_INLINE AuxGeom_AlphaBlendMode::Enum GetAlphaBlendMode() const;
	X_INLINE void SetAlphaBlendMode(const AuxGeom_AlphaBlendMode::Enum state);

	X_INLINE AuxGeom_DrawInFrontMode::Enum GetDrawInFrontMode() const;
	X_INLINE void SetDrawInFrontMode(const AuxGeom_DrawInFrontMode::Enum state);

	X_INLINE AuxGeom_FillMode::Enum GetFillMode() const;
	X_INLINE void SetFillMode(const AuxGeom_FillMode::Enum state);

	X_INLINE AuxGeom_CullMode::Enum GetCullMode() const;
	X_INLINE void SetCullMode(const AuxGeom_CullMode::Enum state);

	X_INLINE AuxGeom_DepthWrite::Enum GetDepthWriteFlag() const;
	X_INLINE void SetDepthWriteFlag(const AuxGeom_DepthWrite::Enum state);

	X_INLINE AuxGeom_DepthTest::Enum GetDepthTestFlag() const;
	X_INLINE void SetDepthTestFlag(const AuxGeom_DepthTest::Enum state);

private:
	uint32 renderFlags_;
};


XAuxGeomRenderFlags::XAuxGeomRenderFlags() : 
	renderFlags_(AuxGeom_Defaults::Def3DRenderflags)
{
}

XAuxGeomRenderFlags::XAuxGeomRenderFlags(const XAuxGeomRenderFlags& rhs) :
renderFlags_(rhs.renderFlags_)
{
}

XAuxGeomRenderFlags::XAuxGeomRenderFlags(uint32 renderFlags) :
	renderFlags_(renderFlags)
{
}

// ----------------------------------------------------------------

XAuxGeomRenderFlags& XAuxGeomRenderFlags::operator =(const XAuxGeomRenderFlags& rhs)
{
	renderFlags_ = rhs.renderFlags_;
	return *this;
}

XAuxGeomRenderFlags& XAuxGeomRenderFlags::operator =(uint32 rhs)
{
	renderFlags_ = rhs;
	return *this;
}

// ----------------------------------------------------------------

bool XAuxGeomRenderFlags::operator ==(const XAuxGeomRenderFlags& rhs) const
{
	return renderFlags_ == rhs.renderFlags_;
}

bool XAuxGeomRenderFlags::operator ==(uint32 rhs) const
{
	return renderFlags_ == rhs;
}

bool XAuxGeomRenderFlags::operator !=(const XAuxGeomRenderFlags& rhs) const
{
	return renderFlags_ != rhs.renderFlags_;
}

bool XAuxGeomRenderFlags::operator !=(uint32 rhs) const
{
	return renderFlags_ != rhs;
}

// ----------------------------------------------------------------

AuxGeom_Mode2D3D::Enum XAuxGeomRenderFlags::GetMode2D3DFlag() const
{
	return (AuxGeom_Mode2D3D::Enum)(renderFlags_ & (uint32)AuxGeomBitMasks::Mode2D3DMask);
}

void XAuxGeomRenderFlags::SetMode2D3DFlag(const AuxGeom_Mode2D3D::Enum state)
{
	renderFlags_ &= ~AuxGeomBitMasks::Mode2D3DMask;
	renderFlags_ |= state; 
}

// ----------------------------------------------------------------

AuxGeom_AlphaBlendMode::Enum XAuxGeomRenderFlags::GetAlphaBlendMode() const
{
	return (AuxGeom_AlphaBlendMode::Enum)(renderFlags_ & (uint32)AuxGeomBitMasks::AlphaBlendingMask);
}

void XAuxGeomRenderFlags::SetAlphaBlendMode(const AuxGeom_AlphaBlendMode::Enum state)
{
	renderFlags_ &= ~AuxGeomBitMasks::AlphaBlendingMask;
	renderFlags_ |= state;
}

// ----------------------------------------------------------------

AuxGeom_DrawInFrontMode::Enum XAuxGeomRenderFlags::GetDrawInFrontMode() const
{
	return (AuxGeom_DrawInFrontMode::Enum)(renderFlags_ & (uint32)AuxGeomBitMasks::DrawInFrontMask);
}

void XAuxGeomRenderFlags::SetDrawInFrontMode(const AuxGeom_DrawInFrontMode::Enum state)
{
	renderFlags_ &= ~AuxGeomBitMasks::DrawInFrontMask;
	renderFlags_ |= state;
}

// ----------------------------------------------------------------

AuxGeom_FillMode::Enum XAuxGeomRenderFlags::GetFillMode() const
{
	return (AuxGeom_FillMode::Enum)(renderFlags_ & (uint32)AuxGeomBitMasks::FillModeMask);
}

void XAuxGeomRenderFlags::SetFillMode(const AuxGeom_FillMode::Enum state)
{
	renderFlags_ &= ~AuxGeomBitMasks::FillModeMask;
	renderFlags_ |= state;
}

// ----------------------------------------------------------------

AuxGeom_CullMode::Enum XAuxGeomRenderFlags::GetCullMode() const
{
	return (AuxGeom_CullMode::Enum)(renderFlags_ & (uint32)AuxGeomBitMasks::CullModeMask);
}

void XAuxGeomRenderFlags::SetCullMode(const AuxGeom_CullMode::Enum state)
{
	renderFlags_ &= ~AuxGeomBitMasks::CullModeMask;
	renderFlags_ |= state;
}

// ----------------------------------------------------------------

AuxGeom_DepthWrite::Enum XAuxGeomRenderFlags::GetDepthWriteFlag() const
{
	return (AuxGeom_DepthWrite::Enum)(renderFlags_ & (uint32)AuxGeomBitMasks::DepthWriteMask);
}

void XAuxGeomRenderFlags::SetDepthWriteFlag(const AuxGeom_DepthWrite::Enum state)
{
	renderFlags_ &= ~AuxGeomBitMasks::DepthWriteMask;
	renderFlags_ |= state;
}

// ----------------------------------------------------------------

AuxGeom_DepthTest::Enum XAuxGeomRenderFlags::GetDepthTestFlag() const
{
	return (AuxGeom_DepthTest::Enum)(renderFlags_ & (uint32)AuxGeomBitMasks::DepthTestMask);
}

void XAuxGeomRenderFlags::SetDepthTestFlag(const AuxGeom_DepthTest::Enum state)
{
	renderFlags_ &= ~AuxGeomBitMasks::DepthTestMask;
	renderFlags_ |= state;
}


X_NAMESPACE_END

#endif // !X_RENDER_AUG_I_H_