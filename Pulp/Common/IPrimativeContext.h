#pragma once

struct Vertex_P3F_T2F_C4B;

#include <IRender.h>
#include <IFont.h>

X_NAMESPACE_BEGIN(engine)


class IPrimativeContext
{
protected:
	typedef render::TopoType PrimitiveType;

public:
	typedef Vertex_P3F_T2F_C4B PrimVertex;

public:
	IPrimativeContext();
	virtual ~IPrimativeContext();

	virtual void reset(void) X_ABSTRACT;

	// Screen Space Draw: range 0-2 width / h is also scrrenspace size not pixels
	void drawQuadSS(float x, float y, float width, float height, const Color& col);
	void drawQuadSS(const Rectf& rect, const Color& col);
	X_INLINE void drawQuadSS(float x, float y, float width, float height, const Color& col, const Color& borderCol);
	X_INLINE void drawQuadImageSS(float x, float y, float width, float height, texture::TexID texture_id, const Color& col);
	void drawQuadImageSS(const Rectf& rect, texture::TexID texture_id, const Color& col);
	X_INLINE void drawRectSS(float x, float y, float width, float height, const Color& col);
	void drawRectSS(const Rectf& rect, const Color& col);
	void drawLineColorSS(const Vec2f& vPos1, const Color& color1,
		const Vec2f& vPos2, const Color& vColor2);

	X_INLINE void drawQuadImage(float x, float y, float width, float height, texture::TexID texture_id, const Color& col);
	X_INLINE void drawQuadImage(float x, float y, float width, float height, texture::ITexture* pTexutre, const Color& col);
	X_INLINE void drawQuadImage(const Rectf& rect, texture::ITexture* pTexutre, const Color& col);

	// for 2d, z is depth not position
	void drawQuad(float x, float y, float z, float width, float height, const Color& col);
	X_INLINE void drawQuad(float x, float y, float z, float width, float height, const Color& col, const Color& borderCol);
	X_INLINE void drawQuad(float x, float y, float width, float height, const Color& col);
	X_INLINE void drawQuad(float x, float y, float width, float height, const Color& col, const Color& borderCol);
	X_INLINE void drawQuad(Vec2<float> pos, float width, float height, const Color& col);
	// draw a quad in 3d z is position not depth.
	void drawQuad3d(const Vec3f& pos0, const Vec3f& pos1, const Vec3f& pos2, const Vec3f& pos3, const Color& col);

	void drawLines(Vec3f* pPoints, uint32_t num, const Color& col);
	void drawLine(const Vec3f& pos1, const Vec3f& pos2);
	void drawLineColor(const Vec3f& pos1, const Color& color1,
		const Vec3f& pos2, const Color& color2);

	void drawRect(float x, float y, float width, float height, const Color& col);

	// ya fucking what!
	void drawBarChart(const Rectf& rect, uint32_t num, float* pHeights,
		float padding, uint32_t max, const Color& col);


	// format buffer is 2048 in size.
	X_INLINE void drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pFormat, va_list args);
	X_INLINE void drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pText);
	X_INLINE void drawText(float x, float y, const font::TextDrawContext& con, const char* pText);
	X_INLINE void drawText(float x, float y, const font::TextDrawContext& con, const char* pText, const char* pEnd);
	virtual void drawText(const Vec3f& poss, const font::TextDrawContext& con, const char* pText, const char* pEnd) X_ABSTRACT;


private:
	X_INLINE void drawImage(float xpos, float ypos, float z, float w, float h,
		texture::TexID texture_id, float s0, float t0, float s1, float t1, const Colorf& col, bool filtered = true);

	void drawImageWithUV(float xpos, float ypos, float z, float w, float h,
		texture::TexID texture_id, const float* s, const float* t, const Colorf& col, bool filtered = true);

public:
	virtual Vertex_P3F_T2F_C4B* addPrimative(uint32_t num, PrimitiveType::Enum type, texture::TexID texture_id) X_ABSTRACT;
	virtual Vertex_P3F_T2F_C4B* addPrimative(uint32_t num, PrimitiveType::Enum type) X_ABSTRACT;

};


X_NAMESPACE_END

#include "IPrimativeContext.inl"