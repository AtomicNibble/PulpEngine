#pragma once


#ifndef X_RENDER_DX12_H_
#define X_RENDER_DX12_H_

#include <IRender.h>

#include "CommandList.h"
#include "SamplerDesc.h"
#include "Allocators\DescriptorAllocator.h"
#include "Allocators\DynamicDescriptorHeap.h"
#include "Buffers\ColorBuffer.h"
#include "Features.h"

#include "Shader\ShaderManager.h"
#include "Vars\RenderVars.h"
#include "Auxiliary\AuxRender.h"


X_NAMESPACE_DECLARE(texture,
	class TextureManager;
)
X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(render)

class RenderAuxImp;

class XRender : public IRender2
{
	static const uint32_t SWAP_CHAIN_BUFFER_COUNT = 3;

	static const DXGI_FORMAT SWAP_CHAIN_FORMAT = DXGI_FORMAT_R10G10B10A2_UNORM;

public:
	XRender(core::MemoryArenaBase* arena);
	~XRender();

	bool init(PLATFORM_HWND hWnd, uint32_t width, uint32_t height) X_OVERRIDE;
	void shutDown(void) X_OVERRIDE;
	void freeResources(void) X_OVERRIDE;

	void registerVars(void) X_OVERRIDE;
	void registerCmds(void) X_OVERRIDE;

	void renderBegin(void) X_OVERRIDE;
	void renderEnd(void) X_OVERRIDE;

	IRenderAux* getAuxRender(AuxRenderer::Enum user) X_OVERRIDE;


private:
	bool freeSwapChainResources(void);

	bool initRenderBuffers(Vec2<uint32_t> res);
	bool resize(uint32_t width, uint32_t height);
	void handleResolutionChange(void);
	void populateFeatureInfo(void);
	bool deviceIsSupported(void) const;

private:
	void Cmd_ListDeviceFeatures(core::IConsoleCmdArgs* pCmd);


private:
	core::MemoryArenaBase* arena_;
	ID3D12Device* pDevice_;
	ID3D12Debug* pDebug_;
	IDXGISwapChain1* pSwapChain_;

	RenderVars vars_;

	shader::XShaderManager shaderMan_; // might make this allocoated, just so can forward declare like tex man.
	texture::TextureManager* pTextureMan_;
	RenderAuxImp* pAuxRender_;

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

	D3D_FEATURE_LEVEL featureLvl_;
	GpuFeatures features_;

	RenderAux auxQues_[AuxRenderer::ENUM_COUNT];
};


X_NAMESPACE_END

#endif // !X_RENDER_DX12_H_