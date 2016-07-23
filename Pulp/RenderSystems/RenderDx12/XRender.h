#pragma once


#ifndef X_RENDER_DX12_H_
#define X_RENDER_DX12_H_

#include <IRender.h>

#include "CommandList.h"
#include "SamplerDesc.h"
#include "DescriptorAllocator.h"
#include "DynamicDescriptorHeap.h"
#include "ColorBuffer.h"

#include "Shader\ShaderManager.h"


X_NAMESPACE_DECLARE(texture,
	class TextureManager;
)

X_NAMESPACE_BEGIN(render)


class XRender : public IRender2
{
	static const uint32_t SWAP_CHAIN_BUFFER_COUNT = 3;

	static const DXGI_FORMAT SWAP_CHAIN_FORMAT = DXGI_FORMAT_R10G10B10A2_UNORM;

public:
	XRender(core::MemoryArenaBase* arena);
	~XRender();

	bool Init(PLATFORM_HWND hWnd, uint32_t width, uint32_t height) X_OVERRIDE;
	void ShutDown(void) X_OVERRIDE;
	void freeResources(void) X_OVERRIDE;

	void RenderBegin(void) X_OVERRIDE;
	void RenderEnd(void) X_OVERRIDE;

private:
	bool FreeSwapChainResources(void);

	bool InitRenderBuffers(Vec2<uint32_t> res);
	bool Resize(uint32_t width, uint32_t height);
	void HandleResolutionChange(void);


private:
	core::MemoryArenaBase* arena_;
	ID3D12Device* pDevice_;
	ID3D12Debug* pDebug_;
	IDXGISwapChain1* pSwapChain_;

	shader::XShaderManager shaderMan_; // might make this allocoated, just so can forward declare like tex man.
	texture::TextureManager* pTextureMan_;

	CommandListManger cmdListManager_;

	core::StackString<128, wchar_t> deviceName_;
	size_t dedicatedvideoMemory_;

	DescriptorAllocator* pDescriptorAllocator_;
	DescriptorAllocatorPool* pDescriptorAllocatorPool_;

	Vec2<uint32_t> currentNativeRes_;
	Vec2<uint32_t> targetNativeRes_;
	Vec2<uint32_t> displayRes_;
	ColorBuffer displayPlane_[SWAP_CHAIN_BUFFER_COUNT];
	uint32_t currentBufferIdx_;

	SamplerDesc samplerLinearWrapDesc_;
	SamplerDesc samplerAnisoWrapDesc_;
	SamplerDesc samplerShadowDesc_;
	SamplerDesc samplerLinearClampDesc_;
	SamplerDesc samplerVolumeWrapDesc_;
	SamplerDesc samplerPointClampDesc_;
	SamplerDesc samplerPointBorderDesc_;
	SamplerDesc samplerLinearBorderDesc_;

	SamplerDescriptor samplerLinearWrap_;
	SamplerDescriptor samplerAnisoWrap_;
	SamplerDescriptor samplerShadow_;
	SamplerDescriptor samplerLinearClamp_;
	SamplerDescriptor samplerVolumeWrap_;
	SamplerDescriptor samplerPointClamp_;
	SamplerDescriptor samplerPointBorder_;
	SamplerDescriptor samplerLinearBorder_;
};


X_NAMESPACE_END

#endif // !X_RENDER_DX12_H_