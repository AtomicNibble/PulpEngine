#pragma once


#ifndef X_RENDER_DX12_H_
#define X_RENDER_DX12_H_

#include <IRender.h>

#include "CommandList.h"

X_NAMESPACE_BEGIN(render)


class XRender : public IRender2
{
	static const uint32_t SWAP_CHAIN_BUFFER_COUNT = 3;

public:
	XRender(core::MemoryArenaBase* arena);
	~XRender();

	bool Init(PLATFORM_HWND hWnd, uint32_t width, uint32_t height) X_OVERRIDE;
	void ShutDown(void) X_OVERRIDE;
	void freeResources(void) X_OVERRIDE;

	void RenderBegin(void) X_OVERRIDE;
	void RenderEnd(void) X_OVERRIDE;

private:
	bool InitRenderBuffers(uint32_t width, uint32_t hegith);
	bool Resize(uint32_t width, uint32_t hegith);


private:
	ID3D12Device* pDevice_;
	ID3D12Debug* pDebug_;

	CommandListManger cmdListManager_;

	core::StackString<128, wchar_t> deviceName_;
	size_t dedicatedvideoMemory_;
};


X_NAMESPACE_END

#endif // !X_RENDER_DX12_H_