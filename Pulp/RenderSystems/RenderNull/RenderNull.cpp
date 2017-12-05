#include "stdafx.h"
#include "RenderNull.h"


X_NAMESPACE_BEGIN(render)

// RenderNull

RenderNull g_NullRender;


bool RenderNull::init(HWND hWnd, 
	uint32_t width, uint32_t hieght, texture::Texturefmt::Enum depthFmt, bool reverseZ)
{
	X_UNUSED(hWnd);
	X_UNUSED(width);
	X_UNUSED(hieght);
	X_UNUSED(depthFmt);
	X_UNUSED(reverseZ);

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

void RenderNull::submitCommandPackets(CommandBucket<uint32_t>& cmdBucket)
{
	X_UNUSED(cmdBucket);

}

Vec2<uint32_t> RenderNull::getDisplayRes(void) const
{
	return Vec2<uint32_t>::zero();
}


IPixelBuffer* RenderNull::createDepthBuffer(const char* pNickName, Vec2i dim)
{
	X_UNUSED(pNickName);
	X_UNUSED(dim);

	return nullptr;
}

IPixelBuffer* RenderNull::createColorBuffer(const char* pNickName, Vec2i dim, uint32_t numMips,
	texture::Texturefmt::Enum fmt)
{
	X_UNUSED(pNickName);
	X_UNUSED(dim);
	X_UNUSED(fmt);
	X_UNUSED(numMips);

	return nullptr;
}
void RenderNull::releasePixelBuffer(render::IPixelBuffer* pPixelBuf)
{
	X_UNUSED(pPixelBuf);

}


IRenderTarget* RenderNull::getCurBackBuffer(uint32_t* pIdx)
{
	X_UNUSED(pIdx);

	return nullptr;
}


VertexBufferHandle RenderNull::createVertexBuffer(uint32_t numElements, uint32_t elementSize,
	BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	X_UNUSED(numElements);
	X_UNUSED(elementSize);
	X_UNUSED(usage);
	X_UNUSED(accessFlag);
	return 0;
}

VertexBufferHandle RenderNull::createVertexBuffer(uint32_t numElements, uint32_t elementSize,
	const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	X_UNUSED(numElements);
	X_UNUSED(elementSize);
	X_UNUSED(pInitialData);
	X_UNUSED(usage);
	X_UNUSED(accessFlag);
	return 0;
}

IndexBufferHandle RenderNull::createIndexBuffer(uint32_t numElements, uint32_t elementSize,
	BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	X_UNUSED(numElements);
	X_UNUSED(elementSize);
	X_UNUSED(usage);
	X_UNUSED(accessFlag);
	return 0;
}

IndexBufferHandle RenderNull::createIndexBuffer(uint32_t numElements, uint32_t elementSize,
	const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	X_UNUSED(numElements);
	X_UNUSED(elementSize);
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


ConstantBufferHandle RenderNull::createConstBuffer(const shader::XCBuffer& cbuffer, BufUsage::Enum usage)
{
	X_UNUSED(cbuffer);
	X_UNUSED(usage);

	return INVALID_BUF_HANLDE;
}

void RenderNull::destoryConstBuffer(ConstantBufferHandle handle)
{
	X_UNUSED(handle);

}


IDeviceTexture* RenderNull::getDeviceTexture(int32_t id)
{
	X_UNUSED(id);

	return nullptr;
}


IDeviceTexture* RenderNull::createTexture(const char* pNickName, Vec2i dim, 
	texture::Texturefmt::Enum fmt, BufUsage::Enum usage, const uint8_t* pInitialData)
{
	X_UNUSED(pNickName, dim, fmt, usage, pInitialData);

	return nullptr;
}


bool RenderNull::initDeviceTexture(IDeviceTexture* pTex)
{
	X_UNUSED(pTex);
	return false;
}

bool RenderNull::initDeviceTexture(IDeviceTexture* pTex, const texture::XTextureFile& imgFile)
{
	X_UNUSED(pTex, imgFile);
	return false;
}



shader::IShaderSource* RenderNull::getShaderSource(const core::string& name)
{
	X_UNUSED(name);

	return nullptr;
}

shader::IHWShader* RenderNull::createHWShader(shader::ShaderType::Enum type, const core::string& entry, const core::string& customDefines,
	shader::IShaderSource* pSourceFile, shader::PermatationFlags permFlags, render::shader::VertexFormat::Enum vertFmt)
{
	X_UNUSED(type);
	X_UNUSED(entry);
	X_UNUSED(customDefines);
	X_UNUSED(pSourceFile);
	X_UNUSED(permFlags, vertFmt);

	return nullptr;
}


shader::IShaderPermatation* RenderNull::createPermatation(const shader::ShaderStagesArr& stages)
{
	X_UNUSED(stages);

	return nullptr;
}

void RenderNull::releaseShaderPermatation(shader::IShaderPermatation* pPerm)
{
	X_UNUSED(pPerm);
}

void RenderNull::releaseTexture(IDeviceTexture* pTex)
{
	X_UNUSED(pTex);

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

StateHandle RenderNull::createState(PassStateHandle passHandle, const shader::IShaderPermatation* pTech, const StateDesc& state,
	const TextureState* pTextStates, size_t numStates)
{
	X_UNUSED(passHandle);
	X_UNUSED(pTech);
	X_UNUSED(state);
	X_UNUSED(pTextStates);
	X_UNUSED(numStates);

	return INVALID_STATE_HANLDE;
}

void RenderNull::destoryState(StateHandle handle)
{
	X_UNUSED(handle);
}




X_NAMESPACE_END
