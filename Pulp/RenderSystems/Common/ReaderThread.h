#pragma once


#ifndef X_RENDER_THREAD_H_
#define X_RENDER_THREAD_H_

X_NAMESPACE_DECLARE(texture, struct XTextureFile;);
X_NAMESPACE_DECLARE(texture, class XTexture;);
X_NAMESPACE_DECLARE(shader, class XShaderResources;);

#include <Threading\Thread.h>
#include <Assets\AssertContainer.h>

#include "RenderAux.h"

X_NAMESPACE_BEGIN(render)

// all the jobs we support baby!
class XBaseAsset;


struct RenderCommand
{
	enum Enum : uint8_t
	{
		UNKOWN,

		CreateDeviceTexture,
		DrawLines,
		DrawString,

		DrawImageWithUV,

		SetState,
		SetCullmode,

		ReleaseBaseResource,
		ReleaseDeviceTexture,
		ReleaseShaderResource,

		SetCameraInfo,

		UpdateTextureRegion,

		FlushTextBuffer,
		AuxGeoFlush,

		Init,
		ShutDown

	};
};


class XRenderThread
{
public:
	XRenderThread();
	~XRenderThread();


	void init();
	void quitRenderThread();
	void quitRenderLoadingThread();
	void syncMainWithRender();
	void processCommands();
	void process(const core::Thread&);         // Render thread
	void processLoading(const core::Thread&);  // Loading thread

	void startRenderThread();
	void startRenderLoadingThread();

	void flushAndWait();
	void waitFlushCond(const core::Thread& t);
	void waitFlushFinishedCond();
	X_INLINE void signalFlushFinishedCond();
	X_INLINE void signalFlushCond();
	X_INLINE void signalQuitCond();
	X_INLINE void initFlushCond();
	X_INLINE bool checkFlushCond();

	X_INLINE int getCurrentThreadId(bool bAlwaysCheck = false);
	X_INLINE bool isRenderThread(bool bAlwaysCheck = false);
	X_INLINE bool isMainThread(bool bAlwaysCheck = false);
	X_INLINE bool isMultithreaded();
	X_INLINE bool isFailed();

	X_INLINE void beginCommand(RenderCommand::Enum RenderCmd, size_t nParamBytes);
	X_INLINE void endCommand();

	template<typename T>
	X_INLINE void write(const T& val);
	X_INLINE void write(const void* pVal, size_t size);

// API
	bool RC_CreateDeviceTexture(
		texture::XTexture* image, texture::XTextureFile* image_data);

	void RC_DrawLines(Vec3f* points, uint32_t num, const Color& col);
	
	void RC_DrawString(const Vec3f& pos, const char* pStr);

	void RC_SetState(StateFlag state);
	void RC_SetCullMode(CullMode::Enum mode);

	void RC_ReleaseBaseResource(core::XBaseAsset* pRes);
	void RC_ReleaseDeviceTexture(texture::XTexture *pTexture);
	void RC_ReleaseShaderResource(shader::XShaderResources* pRes);

	void RC_DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
		texture::TexID texture_id, const float* s, const float* t, const Colorf& col, bool filtered);

	void RT_SetCameraInfo(void);

	void RC_UpdateTextureRegion(texture::XTexture *pTexture, byte* data,
		int nX, int nY, int USize, int VSize, texture::Texturefmt::Enum srcFmt);

	void RC_FlushTextBuffer(void);

	void RC_AuxFlush(IRenderAuxImpl* pAux, const XAuxGeomCBRawDataPackaged& data, size_t begin, size_t end);

private:
	core::Thread m_RenderThread;
	core::Thread m_RenderLoadingThread;

	core::ByteStreamFifo m_Commands;

	volatile int m_nFlush;

	bool m_bSuccessful;
	bool m_bBeginFrameCalled;
	bool m_bEndFrameCalled;
	// Threads
	uint32 m_nRenderThread;
	uint32 m_nRenderThreadLoading;
	uint32 m_nMainThread;
};


X_INLINE bool XRenderThread::isRenderThread(bool bAlwaysCheck)
{
	DWORD d = this->getCurrentThreadId(bAlwaysCheck);
	if (d == m_nRenderThreadLoading || d == m_nRenderThread)
		return true;
	return false;
}

X_INLINE bool XRenderThread::isMainThread(bool bAlwaysCheck)
{
	DWORD d = this->getCurrentThreadId(bAlwaysCheck);
	if (d == m_nMainThread)
		return true;
	return false;
}


X_INLINE bool XRenderThread::isMultithreaded()
{
	return true;
}

X_INLINE bool XRenderThread::isFailed()
{
	return !m_bSuccessful;
}


X_INLINE int XRenderThread::getCurrentThreadId(bool bAlwaysCheck)
{
	if (!bAlwaysCheck && m_nRenderThread == m_nMainThread)
		return m_nRenderThread;
	return ::GetCurrentThreadId();
}

X_INLINE void XRenderThread::beginCommand(RenderCommand::Enum RenderCmd, size_t nParamBytes)
{
	m_Commands.write(RenderCmd);
	m_Commands.resize(m_Commands.size()  + nParamBytes);
}

X_INLINE void XRenderThread::endCommand()
{

}

template<typename T>
X_INLINE void XRenderThread::write(const T& val)
{
	m_Commands.write(val);
}


X_INLINE void XRenderThread::write(const void* pVal, size_t size)
{
	m_Commands.write(pVal, size);
}

// ================ Flush my nipples. ================
X_INLINE void XRenderThread::signalFlushFinishedCond()
{
	m_nFlush = 0;
}

X_INLINE void XRenderThread::signalFlushCond()
{
	m_nFlush = 1;
}

X_INLINE void XRenderThread::signalQuitCond()
{
//	m_RenderThread.Stop();
	m_nFlush = 1;
}

X_INLINE void XRenderThread::initFlushCond()
{
	m_nFlush = 0;
}

X_INLINE bool XRenderThread::checkFlushCond()
{
	return *(int*)&m_nFlush != 0;
}



X_NAMESPACE_END

#endif // X_RENDER_THREAD_H_