#include "stdafx.h"
#include "RenderNull.h"

#include <IFont.h> // XTextDrawConect

X_NAMESPACE_BEGIN(render)

// RenderNull

RenderNull g_NullRender;


bool RenderNull::init(HWND hWnd, 
	uint32_t width, uint32_t hieght)
{
	X_UNUSED(hWnd);
	X_UNUSED(width);
	X_UNUSED(hieght);

	return true;
}

void RenderNull::shutDown()
{

}

void RenderNull::freeResources()
{

}

void RenderNull::release()
{

}

void RenderNull::registerVars()
{

}

void RenderNull::registerCmds()
{

}

void RenderNull::renderBegin()
{

}

void RenderNull::renderEnd()
{

}

void RenderNull::submitCommandPackets(CommandBucket<uint32_t>& cmdBucket, Commands::Key::Type::Enum keyType)
{
	X_UNUSED(cmdBucket);
	X_UNUSED(keyType);
}

IRenderAux* RenderNull::getAuxRender(AuxRenderer::Enum user)
{
	X_UNUSED(user);
	return nullptr;
}

Vec2<uint32_t> RenderNull::getDisplayRes(void) const
{
	return Vec2<uint32_t>::zero();
}

IRenderTarget* RenderNull::createRenderTarget()
{
	return nullptr;
}

void RenderNull::destoryRenderTarget(IRenderTarget* pRT)
{
	X_UNUSED(pRT);
}

IRenderTarget* RenderNull::getCurBackBuffer(uint32_t* pIdx)
{
	X_UNUSED(pIdx);

	return nullptr;
}


VertexBufferHandle RenderNull::createVertexBuffer(uint32_t size,
	BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	X_UNUSED(size);
	X_UNUSED(usage);
	X_UNUSED(accessFlag);
	return 0;
}

VertexBufferHandle RenderNull::createVertexBuffer(uint32_t size,
	const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	X_UNUSED(size);
	X_UNUSED(pInitialData);
	X_UNUSED(usage);
	X_UNUSED(accessFlag);
	return 0;
}

IndexBufferHandle RenderNull::createIndexBuffer(uint32_t size,
	BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	X_UNUSED(size);
	X_UNUSED(usage);
	X_UNUSED(accessFlag);
	return 0;
}

IndexBufferHandle RenderNull::createIndexBuffer(uint32_t size,
	const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	X_UNUSED(size);
	X_UNUSED(pInitialData);
	X_UNUSED(usage);
	X_UNUSED(accessFlag);
	return 0;
}

void RenderNull::destoryVertexBuffer(VertexBufferHandle handle)
{
	X_UNUSED(handle);
}

void RenderNull::destoryIndexBuffer(IndexBufferHandle handle)
{
	X_UNUSED(handle);
}


void RenderNull::getVertexBufferSize(VertexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize)
{
	X_UNUSED(handle);
	X_UNUSED(pOriginal);
	X_UNUSED(pDeviceSize);
}

void RenderNull::getIndexBufferSize(IndexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize)
{
	X_UNUSED(handle);
	X_UNUSED(pOriginal);
	X_UNUSED(pDeviceSize);
}



texture::ITexture* RenderNull::getTexture(const char* path, texture::TextureFlags flags)
{
	X_ASSERT_NOT_NULL(path);
	X_UNUSED(path);
	X_UNUSED(flags);

	return nullptr;
}

shader::IShader* RenderNull::getShader(const char* path)
{
	X_ASSERT_NOT_NULL(path);

	return nullptr;
}

void RenderNull::releaseTexture(texture::ITexture* pTex)
{
	X_UNUSED(pTex);

}

void RenderNull::releaseShader(shader::IShader* pShader)
{
	X_UNUSED(pShader);

}

PassStateHandle RenderNull::createPassState(const RenderTargetFmtsArr& rtfs)
{
	X_UNUSED(rtfs);

	return INVALID_BUF_HANLDE;
}

void RenderNull::destoryPassState(PassStateHandle handle)
{
	X_UNUSED(handle);

}

StateHandle RenderNull::createState(PassStateHandle passHandle, const shader::IShaderTech* pTech, const StateDesc& state,
	const TextureState* pTextStates, size_t numStates)
{
	X_UNUSED(passHandle);
	X_UNUSED(pTech);
	X_UNUSED(state);
	X_UNUSED(pTextStates);
	X_UNUSED(numStates);

	return INVALID_BUF_HANLDE;
}

void RenderNull::destoryState(StateHandle handle)
{
	X_UNUSED(handle);
}

// =====================================================================

//void RenderNull::SetState(StateFlag state)
//{
//	X_UNUSED(state);
//}
//
//void RenderNull::SetCullMode(CullMode::Enum mode)
//{
//	X_UNUSED(mode);
//}

void RenderNull::Set2D(bool value, float znear, float zfar)
{
	X_UNUSED(value);
	X_UNUSED(znear);
	X_UNUSED(zfar);

}



void RenderNull::GetViewport(int* x, int* y, int* width, int* height)
{
	X_ASSERT_NOT_NULL(x);
	X_ASSERT_NOT_NULL(y);
	X_ASSERT_NOT_NULL(width);
	X_ASSERT_NOT_NULL(height);
}

void RenderNull::SetViewport(int x, int y, int width, int height)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
}

void RenderNull::GetViewport(Recti& rect)
{
	X_UNUSED(rect);

}

void RenderNull::SetViewport(const Recti& rect)
{
	X_UNUSED(rect);

}

int RenderNull::getWidth(void) const
{
	return 0; // return 1 maybe as i might divide by this.
}

int RenderNull::getHeight(void) const
{
	return 0;
}

float RenderNull::getWidthf(void) const
{
	return 0.f; // return 1 maybe as i might divide by this.
}

float RenderNull::getHeightf(void) const
{
	return 0.f;
}


float RenderNull::ScaleCoordX(float value) const
{
	X_UNUSED(value);
	return 0.f;
}

float RenderNull::ScaleCoordY(float value) const
{
	X_UNUSED(value);
	return 0.f;
}

void RenderNull::ScaleCoord(float& x, float& y) const
{
	X_UNUSED(x);
	X_UNUSED(y);
}

void RenderNull::ScaleCoord(Vec2f& xy) const
{
	X_UNUSED(xy);
}


void RenderNull::SetCamera(const XCamera& cam)
{
	cam_ = cam;
}

const XCamera& RenderNull::GetCamera()
{
	return cam_;
}

// AuxGeo
IRenderAux* RenderNull::GetIRenderAuxGeo(void)
{
	return nullptr;
}
// ~AuxGeo


// Textures 

void RenderNull::ReleaseTexture(texture::TexID id)
{
	X_UNUSED(id);
}

bool RenderNull::SetTexture(texture::TexID id)
{
	X_UNUSED(id);
	return false;
}

// ~Textures


// Drawing


void RenderNull::DrawQuadSS(float x, float y, float width, float height,
	const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
}


void RenderNull::DrawQuadSS(const Rectf& rect, const Color& col)
{
	X_UNUSED(rect);
	X_UNUSED(col);
}

void RenderNull::DrawQuadSS(float x, float y, float width, float height,
	const Color& col, const Color& borderCol)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
	X_UNUSED(borderCol);
}

void RenderNull::DrawQuadImageSS(float x, float y, float width, float height,
	texture::TexID texture_id, const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(texture_id);
	X_UNUSED(col);
}

void RenderNull::DrawQuadImageSS(const Rectf& rect, texture::TexID texture_id,
	const Color& col)
{
	X_UNUSED(rect);
	X_UNUSED(texture_id);
	X_UNUSED(col);
}

void RenderNull::DrawRectSS(float x, float y, float width, float height,
	const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
}

void RenderNull::DrawRectSS(const Rectf& rect, const Color& col)
{
	X_UNUSED(rect);
	X_UNUSED(col);
}

void RenderNull::DrawLineColorSS(const Vec2f& vPos1, const Color& color1,
	const Vec2f& vPos2, const Color& vColor2)
{
	X_UNUSED(vPos1);
	X_UNUSED(color1);
	X_UNUSED(vPos2);
	X_UNUSED(vColor2);
}

void RenderNull::DrawQuadImage(float x, float y, float width, float height,
	texture::TexID texture_id, const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(texture_id);
	X_UNUSED(col);
}

void RenderNull::DrawQuadImage(float x, float y, float width, float height,
	texture::ITexture* pTexutre, const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(pTexutre);
	X_UNUSED(col);
}

void RenderNull::DrawQuadImage(const Rectf& rect, texture::ITexture* pTexutre,
	const Color& col)
{
	X_UNUSED(rect);
	X_UNUSED(pTexutre);
	X_UNUSED(col);
}




void RenderNull::DrawQuad(float x, float y, float z, float width, float height, const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(z);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
}

void RenderNull::DrawQuad(float x, float y, float z, float width, float height,
	const Color& col, const Color& bordercol)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(z);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
	X_UNUSED(bordercol);
}

void RenderNull::DrawQuad(float x, float y, float width, float height,
	const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
}

void RenderNull::DrawQuad(float x, float y, float width, float height,
	const Color& col, const Color& bordercol)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
	X_UNUSED(bordercol);
}

void RenderNull::DrawQuad(Vec2<float> pos, float width, float height, const Color& col)
{
	X_UNUSED(pos);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);

}

void RenderNull::DrawQuad3d(const Vec3f& pos0, const Vec3f& pos1, const Vec3f& pos2, const Vec3f& pos3, const Color& col)
{
	X_UNUSED(pos0);
	X_UNUSED(pos1);
	X_UNUSED(pos2);
	X_UNUSED(pos3);
	X_UNUSED(col);
}


void RenderNull::DrawLines(Vec3f* points, uint32_t num, const Color& col)
{
	X_ASSERT_NOT_NULL(points);
	X_UNUSED(points);
	X_UNUSED(num);
	X_UNUSED(col);
}

void RenderNull::DrawLine(const Vec3f& pos1, const Vec3f& pos2)
{
	X_UNUSED(pos1);
	X_UNUSED(pos2);

}

void RenderNull::DrawLineColor(const Vec3f& pos1, const Color& color1,
	const Vec3f& pos2, const Color& color2)
{
	X_UNUSED(pos1);
	X_UNUSED(pos2);
	X_UNUSED(color1);
	X_UNUSED(color2);

}

void RenderNull::DrawRect(float x, float y, float width, float height, const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
}


void RenderNull::DrawBarChart(const Rectf& rect, uint32_t num, float* heights,
	float padding, uint32_t max)
{
	X_UNUSED(rect);
	X_UNUSED(num);
	X_UNUSED(heights);
	X_UNUSED(padding);
	X_UNUSED(max);
}

void RenderNull::DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* format, va_list args)
{
	X_UNUSED(pos);
	X_UNUSED(ti);
	X_UNUSED(format);
	X_UNUSED(args);
}

void RenderNull::DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* text)
{
	X_UNUSED(pos);
	X_UNUSED(ti);
	X_UNUSED(text);
}

void RenderNull::DrawAllocStats(Vec3f pos, const XDrawTextInfo& ti,
	const core::MemoryAllocatorStatistics& allocStats, const char* title)
{
	X_UNUSED(pos);
	X_UNUSED(ti);
	X_UNUSED(allocStats);
	X_UNUSED(title);

}

void RenderNull::FlushTextBuffer(void)
{

}

// ~Drawing



// Font

int RenderNull::FontCreateTexture(const Vec2i& size, BYTE* pData,
	texture::Texturefmt::Enum eTF, bool genMips)
{
	X_ASSERT_NOT_NULL(pData);
	X_UNUSED(size);
	X_UNUSED(pData);
	X_UNUSED(eTF);
	X_UNUSED(genMips);


	return 0;
}


bool RenderNull::FontUpdateTexture(int texId, int x, int y, int USize, int VSize,
	uint8_t* pData)
{
	X_ASSERT_NOT_NULL(pData);
	X_UNUSED(texId);
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(USize);
	X_UNUSED(VSize);
	X_UNUSED(pData);

	return false;
}

bool RenderNull::FontSetTexture(int texId)
{
	X_UNUSED(texId);

	return false;
}

bool RenderNull::FontSetRenderingState()
{
	return false;
}

void RenderNull::FontRestoreRenderingState()
{

}

void RenderNull::FontSetBlending()
{

}


void RenderNull::DrawStringW(font::IXFont_RenderProxy* pFont, const Vec3f& pos,
	const wchar_t* pStr, const font::XTextDrawConect& ctx) const
{
	X_ASSERT_NOT_NULL(pFont);
	X_ASSERT_NOT_NULL(pStr);

	X_UNUSED(pFont);
	X_UNUSED(pos);
	X_UNUSED(pStr);
	X_UNUSED(ctx);


}


// ~Font


//void RenderNull::DrawVB(Vertex_P3F_T2F_C4B* pVertBuffer, uint32_t size,
//	PrimitiveTypePublic::Enum type)
//{
//	X_ASSERT_NOT_NULL(pVertBuffer);
//
//	X_UNUSED(pVertBuffer);
//	X_UNUSED(size);
//	X_UNUSED(type);
//
//}

// Shader Stuff

shader::XShaderItem RenderNull::LoadShaderItem(shader::XInputShaderResources& res)
{
	X_UNUSED(res);
	return shader::XShaderItem();
}


bool RenderNull::DefferedBegin(void)
{
	return false;
}

bool RenderNull::DefferedEnd(void)
{
	return false;
}

bool RenderNull::SetWorldShader(void)
{
	return false;
}

bool RenderNull::setGUIShader(bool textured)
{
	X_UNUSED(textured);

	return false;
}

// ~Shader Stuff

// Model


void RenderNull::SetModelMatrix(const Matrix44f& mat)
{
	X_UNUSED(mat);
}

// ~Model



X_NAMESPACE_END
