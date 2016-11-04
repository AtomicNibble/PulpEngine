
X_NAMESPACE_BEGIN(engine)


X_INLINE void IPrimativeContext::drawQuadSS(float x, float y, float width, float height,
	const Color& col, const Color& borderCol)
{
	drawQuadSS(x, y, width, height, col);
	drawRectSS(x, y, width, height, borderCol);
}


X_INLINE void IPrimativeContext::drawQuadImageSS(float x, float y, float width, float height, texture::TexID texture_id, const Color& col)
{
	const Rectf rect(x, y, x + width, y + height);
	drawQuadImageSS(rect, texture_id, col);
}


X_INLINE void IPrimativeContext::drawRectSS(float x, float y, float width, float height, const Color& col)
{
	const Rectf rect(x, y, x + width, y + height);
	drawRectSS(rect, col);
}

X_INLINE void IPrimativeContext::drawQuadImage(float xpos, float ypos,
	float w, float h, texture::TexID texture_id, const Color& col)
{
	drawImage(xpos, ypos, 0.f, w, h, texture_id, 0, 1, 1, 0, col);
}

X_INLINE void IPrimativeContext::drawQuadImage(float xpos, float ypos,
	float w, float h, texture::ITexture* pTexutre, const Color& col)
{
	drawImage(xpos, ypos, 0.f, w, h, pTexutre->getTexID(), 0, 1, 1, 0, col);
}

X_INLINE void IPrimativeContext::drawQuadImage(const Rectf& rect, texture::ITexture* pTexutre,
	const Color& col)
{
	drawImage(rect.getX1(), rect.getY1(), 0.f, rect.getWidth(), rect.getHeight(),
		pTexutre->getTexID(), 0, 1, 1, 0, col);
}

X_INLINE void IPrimativeContext::drawImage(float xpos, float ypos, float z, float w, float h,
	texture::TexID texture_id, float s0, float t0, float s1, float t1,
	const Colorf& col, bool filtered)
{
	float s[4], t[4];

	s[0] = s0;	t[0] = 1.0f - t0;
	s[1] = s1;	t[1] = 1.0f - t0;
	s[2] = s0;	t[2] = 1.0f - t1;
	s[3] = s1;	t[3] = 1.0f - t1;

	drawImageWithUV(xpos, ypos, z, w, h, texture_id, s, t, col, filtered);
}

X_INLINE void IPrimativeContext::drawQuad(float x, float y, float width, float height, const Color& col)
{
	drawQuad(x, y, 0.f, width, height, col);
}

X_INLINE void IPrimativeContext::drawQuad(float x, float y, float width, float height,
	const Color& col, const Color& borderCol)
{
	drawQuad(x, y, 0.f, width, height, col);
	drawRect(x, y, width, height, borderCol);
}

X_INLINE void IPrimativeContext::drawQuad(float x, float y, float z, float width, float height,
	const Color& col, const Color& borderCol)
{
	drawQuad(x, y, z, width, height, col);
	drawRect(x, y, width, height, borderCol);
}

X_INLINE void IPrimativeContext::drawQuad(Vec2<float> pos, float width, float height, const Color& col)
{
	drawQuad(pos.x, pos.y, width, height, col);
}


X_INLINE void IPrimativeContext::drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pFormat, va_list args)
{
	core::StackString<2048> temp;
	temp.appendFmt(pFormat, args);

	drawText(pos, con, temp.begin(), temp.end());
}

X_INLINE void IPrimativeContext::drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pText)
{
	drawText(pos, con, pText, pText + core::strUtil::strlen(pText));
}

X_INLINE void IPrimativeContext::drawText(float x, float y, const font::TextDrawContext& con, const char* pText)
{
	drawText(Vec3f(x, y, 1), con, pText, pText + core::strUtil::strlen(pText));
}

X_INLINE void IPrimativeContext::drawText(float x, float y, const font::TextDrawContext& con, const char* pText, const char* pEnd)
{
	drawText(Vec3f(x, y, 1 ), con, pText, pEnd);
}


X_NAMESPACE_END