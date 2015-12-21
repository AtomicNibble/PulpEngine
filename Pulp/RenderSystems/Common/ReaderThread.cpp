#include "stdafx.h"
#include "ReaderThread.h"

#include "Textures\XTextureFile.h"
#include "Textures\XTexture.h"
#include "XRender.h"

#include <ITimer.h>
#include <IShader.h>



X_NAMESPACE_BEGIN(render)

namespace
{
	static core::Thread::ReturnValue process_run(const core::Thread& t)
	{
		X_UNUSED(t);
	//	gRenDev->rThread()->process(t);

		return core::Thread::ReturnValue(0);
	}

	static core::Thread::ReturnValue processLoading_run(const core::Thread& t)
	{
		X_UNUSED(t);
	//	gRenDev->rThread()->processLoading(t);

		return core::Thread::ReturnValue(0);
	}

}


XRenderThread::XRenderThread() : 
m_Commands(g_rendererArena)
{
	X_ASSERT_NOT_NULL(g_rendererArena);

	m_bSuccessful = true;
	m_bBeginFrameCalled = false;
	m_bEndFrameCalled = false;

	m_nRenderThread = 0;
	m_nRenderThreadLoading = 0;
	m_nMainThread = 0;

	m_Commands.resize(1024 * 200);

	init();
}

XRenderThread::~XRenderThread()
{
	quitRenderThread();
	quitRenderLoadingThread();

}


// thread Stop
void XRenderThread::quitRenderThread()
{
	if (isMultithreaded())
	{
		signalQuitCond();
		m_RenderThread.Stop();
		m_RenderThread.Join();
	}
}

void XRenderThread::quitRenderLoadingThread()
{
	if (isMultithreaded())
	{
		signalQuitCond();
		m_RenderLoadingThread.Stop();
		m_RenderLoadingThread.Join();
	}
}

// -------- Init -------
void XRenderThread::init()
{
	m_nRenderThread = core::Thread::GetCurrentID();
	m_nMainThread = m_nRenderThread;
	m_bSuccessful = true;

	initFlushCond();


	m_RenderThread.Create("RenderThread", 64*1024);
	m_RenderLoadingThread.Create("RenderThread2");
}


void XRenderThread::startRenderThread()
{
	m_RenderThread.Start(process_run);
}

void XRenderThread::startRenderLoadingThread()
{
	m_RenderThread.Start(processLoading_run);
}

// -------- Thread Functions -------------
void XRenderThread::process(const core::Thread& t)
{
	X_DISABLE_WARNING(4127)
	while (true)
	X_ENABLE_WARNING(4127)
	{
		waitFlushCond(t);

		if (!t.ShouldRun())
		{
			signalFlushFinishedCond();
			break;
		}

		processCommands();
		signalFlushFinishedCond();
	}
}

void XRenderThread::processLoading(const core::Thread& t)
{
	X_DISABLE_WARNING(4127)
	while (true)
	{
		waitFlushCond(t);

		if (!t.ShouldRun())
		{
			signalFlushFinishedCond();
			break;
		}

		processCommands();
	}
	X_ENABLE_WARNING(4127)
}


// -------- API Functions -------------

bool XRenderThread::RC_CreateDeviceTexture(
	texture::XTexture* image,  texture::XTextureFile* image_data)
{
	if (isRenderThread())
	{
		// do it now.
		return image->RT_CreateDeviceTexture(image_data);
	}
	
	beginCommand(RenderCommand::CreateDeviceTexture, sizeof(void*) * 2);

	write(image);
	write(image_data);

	endCommand();
	return true;
}


void XRenderThread::RC_DrawLines(Vec3f* points, uint32_t num, const Colorf& col)
{
	if (isRenderThread())
	{
		gRenDev->RT_DrawLines(points, num, col);
		return;
	}

	const size_t CmdSize = (sizeof(int) * 2) + sizeof(Colorf) + sizeof(float) +
		sizeof(Vec3f) * num;
	
	beginCommand(RenderCommand::DrawLines, CmdSize);

	write(num);
	write(col);
	write(points, num * sizeof(Vec3f));

	endCommand();
}


void XRenderThread::RC_DrawString(const Vec3f& pos, const char* pStr)
{
	if (isRenderThread())
	{
		gRenDev->RT_DrawString(pos, pStr);
		return;
	}

	const size_t str_len = core::strUtil::strlen(pStr);
	const size_t CmdSize = sizeof(Vec3f) + str_len + sizeof(str_len);

	beginCommand(RenderCommand::DrawString, CmdSize);

	write(pos);
	write(str_len);
	write(pStr, str_len);

	endCommand();
}

void XRenderThread::RC_SetState(StateFlag state)
{
	if (isRenderThread())
	{
		gRenDev->RT_SetState(state);
		return;
	}

	beginCommand(RenderCommand::SetState, sizeof(state));
	
		write(state);

	endCommand();
}

void XRenderThread::RC_SetCullMode(CullMode::Enum mode)
{
	if (isRenderThread())
	{
		gRenDev->RT_SetCullMode(mode);
		return;
	}

	beginCommand(RenderCommand::SetCullmode, sizeof(mode));

		write(mode);

	endCommand();
}


void XRenderThread::RC_ReleaseBaseResource(core::XBaseAsset* pRes)
{
	X_ASSERT_NOT_NULL(pRes);

	if (isRenderThread()) 
	{
//		texture::XTexture *derrived = static_cast<texture::XTexture*>(pRes)
	//	delete pRes;
	//	X_DELETE( pRes, g_rendererArena);
		X_WARNING("baseRes","can't free");
		return;
	}

	beginCommand(RenderCommand::ReleaseBaseResource, sizeof(pRes));

		write(pRes);

	endCommand();
}

void XRenderThread::RC_ReleaseDeviceTexture(texture::XTexture* pTexture)
{
	X_ASSERT_NOT_NULL(pTexture);

	if (isRenderThread()) {
		pTexture->RT_ReleaseDevice();
		return;
	}

	beginCommand(RenderCommand::ReleaseDeviceTexture, sizeof(pTexture));

	write(pTexture);

	endCommand();
}

void XRenderThread::RC_ReleaseShaderResource(shader::XShaderResources* pRes)
{
	X_ASSERT_NOT_NULL(pRes);

	if (isRenderThread()) {
		pRes->RT_Release();
		return;
	}

	beginCommand(RenderCommand::ReleaseShaderResource, sizeof(pRes));

	write(pRes);

	endCommand();
}

void XRenderThread::RC_DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
	texture::TexID texture_id, const float* s, const float* t, const Colorf& col, bool filtered)
{
	X_ASSERT_NOT_NULL(s);
	X_ASSERT_NOT_NULL(t);

	if (isRenderThread()) {
		gRenDev->RT_DrawImageWithUV(xpos, ypos, z, w, h, texture_id, s, t,
			col, filtered);
		return;
	}

	const size_t size = (sizeof(float)* (5 + 8)) + sizeof(int)+sizeof(Colorf)+sizeof(bool);
	beginCommand(RenderCommand::DrawImageWithUV, size);

	write(xpos);
	write(ypos);
	write(z);
	write(w);
	write(h);
	write(texture_id);

	write(s, sizeof(float)* 4);
	write(t, sizeof(float)* 4);

	write(col);
	write(filtered);


	endCommand();

}

void XRenderThread::RT_SetCameraInfo(void)
{
	if (isRenderThread()) {
		gRenDev->RT_SetCameraInfo();
		return;
	}

	beginCommand(RenderCommand::SetCameraInfo, 0);

	// no prams.

	endCommand();
}

void XRenderThread::RC_UpdateTextureRegion(texture::XTexture* pTexture, 
	byte* pData,
	int nX, int nY, int USize, int VSize, 
	texture::Texturefmt::Enum srcFmt)
{
	X_ASSERT_NOT_NULL(pTexture);
	X_ASSERT_NOT_NULL(pData);

	if (isRenderThread()) {
		pTexture->RT_UpdateTextureRegion(pData, nX, nY, USize, VSize, srcFmt);
		return;
	}

	const size_t size = sizeof(texture::XTexture*)+(sizeof(int)* 4)+sizeof(srcFmt)+sizeof(byte*);
	beginCommand(RenderCommand::UpdateTextureRegion, size);

	write(pTexture);
	write(pData);
	write(nX);
	write(nY);
	write(USize);
	write(VSize);
	write(srcFmt);

	endCommand();
}


void XRenderThread::RC_FlushTextBuffer(void)
{
	if (isRenderThread()) {
		gRenDev->RT_FlushTextBuffer();
		return;
	}

	beginCommand(RenderCommand::FlushTextBuffer, 0);

	// no goats here

	endCommand();
}

void XRenderThread::RC_AuxFlush(IRenderAuxImpl* pAux, 
	const XAuxGeomCBRawDataPackaged& data, size_t begin, size_t end)
{
	X_ASSERT_NOT_NULL(pAux);

	if (isRenderThread()) {
		pAux->RT_Flush(data,begin,end);
		return;
	}

	size_t size = sizeof(data) + sizeof(begin) + sizeof(end);

	beginCommand(RenderCommand::AuxGeoFlush, size);

	write(data.pData_);
	write(begin);
	write(end);

	endCommand();
}


// ----------------------- do the work slut! -------------------


void XRenderThread::processCommands()
{
	X_ASSERT(isRenderThread(), "must be render thread")();

	using namespace texture;

	while (m_Commands.size())
	{
		RenderCommand::Enum action = m_Commands.read<RenderCommand::Enum>();

		switch (action)
		{
			case RenderCommand::CreateDeviceTexture:
			{
				XTexture* image = m_Commands.read<XTexture*>();
				XTextureFile* image_data = m_Commands.read<XTextureFile*>();

				image->RT_CreateDeviceTexture(image_data);
				
				break;
			}

			case RenderCommand::DrawLines:
			{
				int num = m_Commands.read<int>();
				Colorf col = m_Commands.read<Colorf>();

				Vec3f* V = (Vec3f*)&m_Commands.peek<Vec3f>();

				// skip the array.
				m_Commands.seek(num * sizeof(Vec3f));

				gRenDev->RT_DrawLines(V, num, col);
				break;
			}

			case RenderCommand::DrawString:
			{
				Vec3f pos = m_Commands.read<Vec3f>();
				uint32_t str_len = m_Commands.read<uint32_t>();

				char* str = (char*)&m_Commands.peek<char>();

				// skip string.
				m_Commands.seek(str_len);

				gRenDev->RT_DrawString(pos, str);
				break;
			}

			case RenderCommand::SetState:
			{
				StateFlag state = m_Commands.read<StateFlag>();
				
				gRenDev->RT_SetState(state);
				break;
			}

			case RenderCommand::SetCullmode:
			{
				CullMode::Enum mode = m_Commands.read<CullMode::Enum>();
				
				gRenDev->RT_SetCullMode(mode);
				break;
			}

			case RenderCommand::ReleaseBaseResource:
			{
				core::XBaseAsset* pRes = m_Commands.read<core::XBaseAsset*>();
				X_DELETE(pRes, g_rendererArena);
				break;
			}

			case RenderCommand::ReleaseDeviceTexture:
			{
				texture::XTexture* pRes = m_Commands.read<texture::XTexture*>();
				pRes->RT_ReleaseDevice();
				break;
			}

			case RenderCommand::DrawImageWithUV:
			{
				float xpos = m_Commands.read<float>();
				float ypos = m_Commands.read<float>();
				float z = m_Commands.read<float>();
				float w = m_Commands.read<float>();
				float h = m_Commands.read<float>();
				texture::TexID tex_id = m_Commands.read<texture::TexID>();
				
				float* s = &m_Commands.peek<float>();
				m_Commands.skip<float>(4);
				float* t = &m_Commands.peek<float>();
				m_Commands.skip<float>(4);

				Colorf col = m_Commands.read<Colorf>();
				bool filterd = m_Commands.read<bool>();

				gRenDev->RT_DrawImageWithUV(
						xpos,
						ypos,
						z,
						w,
						h,
						tex_id,
						s,
						t,
						col,
						filterd			
					);

				break;
			}

			case RenderCommand::SetCameraInfo:
			{
				gRenDev->RT_SetCameraInfo();
				break;
			}

			case RenderCommand::UpdateTextureRegion:
			{
				texture::XTexture* pTex = m_Commands.read<texture::XTexture*>();
				byte* pData = m_Commands.read<byte*>();
				int nX = m_Commands.read<int>();
				int nY = m_Commands.read<int>();
				int USize = m_Commands.read<int>();
				int VSize = m_Commands.read<int>();
				texture::Texturefmt::Enum srcFmt = m_Commands.read<texture::Texturefmt::Enum>();

				pTex->RT_UpdateTextureRegion(pData, nX, nY, USize, VSize, srcFmt);
				break;
			}
				
			case RenderCommand::FlushTextBuffer:
			{
				gRenDev->RT_FlushTextBuffer();
				break;
			}


			case RenderCommand::AuxGeoFlush:
			{
				IRenderAuxImpl* pAux = m_Commands.read<IRenderAuxImpl*>();
				const XRenderAux::XAuxGeomCBRawData* pData = m_Commands.read<XRenderAux::XAuxGeomCBRawData*>();
				size_t begin = m_Commands.read<size_t>();
				size_t end = m_Commands.read<size_t>();

				pAux->RT_Flush(XAuxGeomCBRawDataPackaged(pData),begin,end);
				break;
			}

#if X_DEBUG 
			default:
				X_ASSERT_UNREACHABLE();
				break;
#else
				X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
		}

	}
}

// ---------- Flush util ---------

void XRenderThread::flushAndWait()
{
	if (isRenderThread())
		return;

	syncMainWithRender();
	syncMainWithRender();
}

void XRenderThread::syncMainWithRender()
{
	if (!isMultithreaded()){
		return;
	}

	waitFlushFinishedCond();

	// m_nCurThreadProcess = m_nCurThreadFill;


	signalFlushCond();
}

void XRenderThread::waitFlushCond(const core::Thread& t)
{
	MemoryBarrier();
	while (!*(volatile int*)&m_nFlush)
	{
		if (!t.ShouldRun())
			break;
		::Sleep(6);
		MemoryBarrier();
	}
}

void XRenderThread::waitFlushFinishedCond()
{
	while (*(volatile int*)&m_nFlush)
	{
	//	if (GetRenderWindowHandle())
		{
		//	MSG msg;
		//	while (PeekMessageW(&msg, GetRenderWindowHandle(), 0, 0, PM_REMOVE))
			{
		//		TranslateMessage(&msg);
		//		DispatchMessage(&msg);
			}
		}
		Sleep(6);
	}
}




bool XRender::FlushRenderThreadCommands(bool wait)
{
	if (wait) {
//		pRt_->flushAndWait();
	}
	return true;
}



X_NAMESPACE_END