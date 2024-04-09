
X_NAMESPACE_BEGIN(engine)

X_INLINE void IPrimitiveContext::drawQuad(float xpos, float ypos,
    float w, float h, Material* pMaterial, Color8u col)
{
    drawImage(xpos, ypos, 0.f, w, h, pMaterial, 0, 1, 1, 0, col);
}

X_INLINE void IPrimitiveContext::drawQuad(const Rectf& rect, Material* pMaterial,
    Color8u col)
{
    drawImage(rect.getX1(), rect.getY1(), 0.f, rect.getWidth(), rect.getHeight(),
        pMaterial, 0, 1, 1, 0, col);
}

X_INLINE void IPrimitiveContext::drawQuad(const Rectf& rect, Color8u col)
{
    drawQuad(rect.getX1(), rect.getY1(), 0.f, rect.getWidth(), rect.getHeight(), col);
}


X_INLINE void IPrimitiveContext::drawImage(float xpos, float ypos, float z, float w, float h,
    Material* pMaterial, float s0, float t0, float s1, float t1,
    const Colorf& col, bool filtered)
{
    CoordArr s, t;

    s[0] = s0;
    t[0] = 1.0f - t0;
    s[1] = s1;
    t[1] = 1.0f - t0;
    s[2] = s0;
    t[2] = 1.0f - t1;
    s[3] = s1;
    t[3] = 1.0f - t1;

    drawImageWithUV(xpos, ypos, z, w, h, pMaterial, s, t, col, filtered);
}

X_INLINE void IPrimitiveContext::drawQuad(float x, float y, float width, float height, Color8u col)
{
    drawQuad(x, y, 0.f, width, height, col);
}

X_INLINE void IPrimitiveContext::drawQuad(float x, float y, float width, float height,
    Color8u col, Color8u borderCol)
{
    drawQuad(x, y, 0.f, width, height, col);
    drawRect(x, y, width, height, borderCol);
}

X_INLINE void IPrimitiveContext::drawQuad(float x, float y, float z, float width, float height,
    Color8u col, Color8u borderCol)
{
    drawQuad(x, y, z, width, height, col);
    drawRect(x, y, width, height, borderCol);
}

X_INLINE void IPrimitiveContext::drawQuad(Vec2<float> pos, float width, float height, Color8u col)
{
    drawQuad(pos.x, pos.y, width, height, col);
}

X_INLINE void IPrimitiveContext::drawQuad(Vec2<float> pos, Vec2<float> size, Color8u col)
{
    drawQuad(pos.x, pos.y, size.x, size.y, col);
}

// Line
X_INLINE void IPrimitiveContext::drawLine(const Vec3f& pos1, const Vec3f& pos2)
{
    PrimVertex* pLine = addPrimitive(2, PrimitiveType::LINELIST);

    pLine[0].pos = pos1;
    pLine[0].color = Color8u::white();
    pLine[0].st = core::XHalf2::zero();

    pLine[1].pos = pos2;
    pLine[1].color = Color8u::white();
    pLine[1].st = core::XHalf2::zero();
}

X_INLINE void IPrimitiveContext::drawLine(const Vec3f& pos1, Color8u color1, const Vec3f& pos2, Color8u color2)
{
    PrimVertex* pLine = addPrimitive(2, PrimitiveType::LINELIST);

    pLine[0].pos = pos1;
    pLine[0].color = color1;
    pLine[1].pos = pos2;
    pLine[1].color = color2;
}

X_INLINE void IPrimitiveContext::drawLine(const Vec3f& pos1, const Vec3f& pos2, Color8u color1)
{
    PrimVertex* pLine = addPrimitive(2, PrimitiveType::LINELIST);

    pLine[0].pos = pos1;
    pLine[0].color = color1;
    pLine[1].pos = pos2;
    pLine[1].color = color1;
}

X_INLINE void IPrimitiveContext::drawRect(const Rectf& rect, Color8u col)
{
    drawRect(rect.getX1(), rect.getY1(), rect.getWidth(), rect.getHeight(), col);
}

X_INLINE void IPrimitiveContext::drawRect(const Vec2f& tl, const Vec2f& size, Color8u col)
{
    drawRect(tl.x, tl.y, size.x, size.y, col);
}

// Points
X_INLINE void IPrimitiveContext::drawPoint(const Vec3f& pos, Color8u col, uint8_t size)
{
    X_UNUSED(size);

    PrimVertex* pVertices = addPrimitive(1, PrimitiveType::POINTLIST);

    pVertices->pos = pos;
    pVertices->color = col;
}

X_INLINE void IPrimitiveContext::drawPoints(const Vec3f* pPoints, uint32_t numPoints, Color8u col, uint8_t size)
{
    X_UNUSED(size);

    PrimVertex* pVertices = addPrimitive(numPoints, PrimitiveType::POINTLIST);

    for (uint32 i = 0; i < numPoints; ++i) {
        pVertices[i].pos = pPoints[i];
        pVertices[i].color = col;
    }
}

X_INLINE void IPrimitiveContext::drawPoints(const Vec3f* pPoints, uint32_t numPoints, const Color8u* pCol, uint8_t size)
{
    X_UNUSED(size);

    PrimVertex* pVertices = addPrimitive(numPoints, PrimitiveType::POINTLIST);

    for (uint32 i = 0; i < numPoints; ++i) {
        pVertices[i].pos = pPoints[i];
        pVertices[i].color = pCol[i];
    }
}

// Triangle
X_INLINE void IPrimitiveContext::drawTriangle(const Vec3f& v0, Color8u col0,
    const Vec3f& v1, Color8u col1,
    const Vec3f& v2, Color8u col2)
{
    PrimVertex* pVertices = addPrimitive(3, PrimitiveType::TRIANGLELIST);

    pVertices[0].pos = v0;
    pVertices[0].color = col0;

    pVertices[1].pos = v1;
    pVertices[1].color = col1;

    pVertices[2].pos = v2;
    pVertices[2].color = col2;
}

X_INLINE void IPrimitiveContext::drawTriangle(const Vec3f& v0, const Vec3f& v1, const Vec3f& v2, Color8u col)
{
    PrimVertex* pVertices = addPrimitive(3, PrimitiveType::TRIANGLELIST);

    pVertices[0].pos = v0;
    pVertices[0].color = col;

    pVertices[1].pos = v1;
    pVertices[1].color = col;

    pVertices[2].pos = v2;
    pVertices[2].color = col;
}

X_INLINE void IPrimitiveContext::drawBone(const Matrix44f& rParent, const Matrix44f& rBone, Color8u col)
{
    // just make them quat trans cus yer.
    drawBone(Transformf(rParent), Transformf(rBone), col);
}

X_INLINE void IPrimitiveContext::drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pFormat, va_list args)
{
    core::StackString<2048> temp;
    temp.appendFmt(pFormat, args);

    drawText(pos, con, temp.begin(), temp.end());
}

X_INLINE void IPrimitiveContext::drawText(const Vec3f& pos, const font::TextDrawContext& con, const char* pText)
{
    drawText(pos, con, pText, pText + core::strUtil::strlen(pText));
}

X_INLINE void IPrimitiveContext::drawText(const Vec3f& pos, const Matrix33f& ang, const font::TextDrawContext& con, const char* pText)
{
    drawText(pos, ang, con, pText, pText + core::strUtil::strlen(pText));
}

X_INLINE void IPrimitiveContext::drawText(float x, float y, const font::TextDrawContext& con, const char* pText)
{
    drawText(Vec3f(x, y, 1), con, pText, pText + core::strUtil::strlen(pText));
}

X_INLINE void IPrimitiveContext::drawText(float x, float y, const font::TextDrawContext& con, core::string_view text)
{
    drawText(Vec3f(x, y, 1), con, text.begin(), text.end());
}

X_INLINE void IPrimitiveContext::drawText(float x, float y, const font::TextDrawContext& con, const char* pText, const char* pEnd)
{
    drawText(Vec3f(x, y, 1), con, pText, pEnd);
}

X_NAMESPACE_END
