#include "stdafx.h"
#include "Dx10Shader.h"
#include "Dx10Render.h"

#include <IFileSys.h>
#include <ITimer.h>
#include <IConsole.h>
#include <Util\BitUtil.h>
#include <Time\StopWatch.h>

#include <Threading\JobSystem2.h>

#include "../Common/XRender.h"

#include <D3Dcompiler.h>
#include <../../3rdparty/source/directx/D3DX9Shader.h>

X_LINK_LIB("d3dcompiler")
X_LINK_LIB("dxguid")


X_NAMESPACE_BEGIN(shader)

XHWShader_Dx10* XHWShader_Dx10::pCurVS_ = nullptr;
XHWShader_Dx10* XHWShader_Dx10::pCurPS_ = nullptr;
XHWShader_Dx10* XHWShader_Dx10::pCurGS_ = nullptr;

ILTreeNode XHWShader_Dx10::ILTree_;
XShaderBin XHWShader_Dx10::bin_;

ID3D11Buffer** XHWShader_Dx10::pConstBuffers_[ShaderType::ENUM_COUNT][ConstbufType::Num] = { nullptr };
ID3D11Buffer*  XHWShader_Dx10::pCurRequestCB_[ShaderType::ENUM_COUNT][ConstbufType::Num] = { nullptr };
Vec4f* XHWShader_Dx10::pConstBuffData_[ShaderType::ENUM_COUNT][ConstbufType::Num] = { nullptr };
int XHWShader_Dx10::curMaxVecs_[ShaderType::ENUM_COUNT][ConstbufType::Num] = { 0 };
int XHWShader_Dx10::perFrameMaxVecs_[ShaderType::ENUM_COUNT] = { 0 };
int XHWShader_Dx10::perInstantMaxVecs_[ShaderType::ENUM_COUNT] = { 0 };

core::Array<XShaderParam> XHWShader_Dx10::perFrameParams_[ShaderType::ENUM_COUNT] = {0,0,0,0};
core::Array<XShaderParam> XHWShader_Dx10::perInstantParams_[ShaderType::ENUM_COUNT] = { 0, 0, 0, 0 };

int XHWShader_Dx10::writeMergedSource_ = 0;
int XHWShader_Dx10::asyncShaderCompile_ = 0;


namespace
{
	// buffer used for building pram data.
	X_ALIGNED_SYMBOL(Vec4f vecTemp[32], 16);



	X_INLINE void getworldToCameraMatrix(render::DX11XRender* r)
	{
		Matrix44f* pMat = reinterpret_cast<Matrix44f*>(&vecTemp[0]);

		r->GetModelViewMatrix(pMat);

	
	}

	X_INLINE void getworldToScreenMatrix(render::DX11XRender* r)
	{
		Matrix44f* pMat = reinterpret_cast<Matrix44f*>(&vecTemp[0]);

		Matrix44f View;
		Matrix44f Projection;

		r->GetModelViewMatrix(&View);
		r->GetProjectionMatrix(&Projection);

		*pMat = Projection * View;
		pMat->transpose();
	}

	X_INLINE void getcameraToWorldMatrix(render::DX11XRender* r)
	{
		Matrix44f* pMat = reinterpret_cast<Matrix44f*>(&vecTemp[0]);

		r->GetModelViewMatrix(pMat);

		pMat->inverted();
		pMat->transpose();
	}


	X_INLINE void getWorldViewProjectionMatrix(render::DX11XRender* r)
	{
		Matrix44f* pMat = reinterpret_cast<Matrix44f*>(&vecTemp[0]);

		Matrix44f World;
		Matrix44f View;
		Matrix44f Projection;

		// World
		Matrix44f trans = Matrix44f::createTranslation(Vec3f(0, 0, 0));

		World = trans;

		r->GetModelViewMatrix(&View);
		r->GetProjectionMatrix(&Projection);

		*pMat = Projection * View * World;
		pMat->transpose();
	}

	X_INLINE void getObjectToWorldMatrix(render::DX11XRender* r)
	{
		X_UNUSED(r);
		Matrix44f* pMat = reinterpret_cast<Matrix44f*>(&vecTemp[0]);

		pMat->setToIdentity();
		pMat->transpose();
	}

	X_INLINE void getWorldMatrix(render::DX11XRender* r)
	{
		X_ASSERT_NOT_IMPLEMENTED();
		X_UNUSED(r);
	}

	X_INLINE void getviewMatrix(render::DX11XRender* r)
	{
		Matrix44f* pMat = reinterpret_cast<Matrix44f*>(&vecTemp[0]);

		r->GetModelViewMatrix(pMat);
		pMat->transpose();
	}

	X_INLINE void getProjectionMatrix(render::DX11XRender* r)
	{
		Matrix44f* pMat = reinterpret_cast<Matrix44f*>(&vecTemp[0]);

		r->GetProjectionMatrix(pMat);
		pMat->transpose();
	}




	X_INLINE void getTime(render::DX11XRender* r)
	{
		X_UNUSED(r);
		X_ASSERT_NOT_IMPLEMENTED();
		vecTemp[0][0] = 1.f; // / gEnv->pTimer->GetFrameTime();
	}

	X_INLINE void getFrameTime(render::DX11XRender* r)
	{
		X_UNUSED(r);
		X_ASSERT_NOT_IMPLEMENTED();
		vecTemp[0][0] = 1.f; // / gEnv->pTimer->GetFrameTime();
	}

	X_INLINE void getCameraPos(render::DX11XRender* r)
	{
		Vec3f pos(r->GetCamera().getPosition());

		vecTemp[0] = pos;
	}

	X_INLINE void getScreenSize(render::DX11XRender* r)
	{
		float w = r->getWidthf();
		float h = r->getHeightf();


		vecTemp[0].x = w;
		vecTemp[0].y = h;
		vecTemp[0].z = 0.5f / w;
		vecTemp[0].w = 0.5f / h;
	}


	X_INLINE void getCameraFront(render::DX11XRender* r)
	{
		X_ASSERT_NOT_IMPLEMENTED();
		X_UNUSED(r);
	}

	X_INLINE void getCameraRight(render::DX11XRender* r)
	{
		X_ASSERT_NOT_IMPLEMENTED();
		X_UNUSED(r);
	}

	X_INLINE void getCameraUp(render::DX11XRender* r)
	{
		X_ASSERT_NOT_IMPLEMENTED();
		X_UNUSED(r);
	}


	// we need a list of chicken ready for dippin!
	struct XParamDB
	{
		const char* name;
		ParamType::Enum type;
		Flags<ParamFlags> flags;
		XParamDB()
		{
			name = nullptr;
			type = ParamType::Unknown;
		}
		XParamDB(const char* inName, ParamType::Enum inType)
		{
			name = inName;
			type = inType;
		}

		XParamDB(const char* inName, ParamType::Enum inType, ParamFlags::Enum inFlags) :
			XParamDB(inName, inType)
		{
			flags = inFlags;
		}
	};

	// GL = global
	// PB = Per-Batch
	// PI = Per-Instance
	// SI = Per-Instance Static
	// PF = Per-Frame
	// PM = Per-Material
	// SK = Skin data

	// used for matching sematics to types.
	static XParamDB g_SematicParams[] =
	{
	//	XParamDB("ViewProjMatrix", ParamType::PF_ViewProjMatrix),
	//	XParamDB("ViewProjMatrix", ParamType::PF_ViewProjMatrix),

		XParamDB("worldToScreenMatrix", ParamType::PF_worldToScreenMatrix, ParamFlags::MATRIX),
		XParamDB("worldToCameraMatrix", ParamType::PF_worldToCameraMatrix, ParamFlags::MATRIX),
		XParamDB("cameraToWorldMatrix", ParamType::PF_cameraToWorldMatrix, ParamFlags::MATRIX),
		XParamDB("objectToWorldMatrix", ParamType::PI_objectToWorldMatrix, ParamFlags::MATRIX),

		XParamDB("worldMatrix", ParamType::PI_worldMatrix, ParamFlags::MATRIX),
		XParamDB("viewMatrix", ParamType::PI_viewMatrix, ParamFlags::MATRIX),
		XParamDB("projectionMatrix", ParamType::PI_projectionMatrix, ParamFlags::MATRIX),
		XParamDB("worldViewProjectionMatrix", ParamType::PI_worldViewProjectionMatrix, ParamFlags::MATRIX),

		

		XParamDB("time", ParamType::PF_Time, ParamFlags::FLOAT),
		XParamDB("frameTime", ParamType::PF_FrameTime, ParamFlags::FLOAT),
		XParamDB("screensize", ParamType::PF_ScreenSize, ParamFlags::VEC4),
		XParamDB("cameraPos", ParamType::PF_CameraPos),

		XParamDB("cameraFront", ParamType::PB_CameraFront),
		XParamDB("cameraRight", ParamType::PB_CameraRight),
		XParamDB("cameraUp", ParamType::PB_CameraUp),

	};

	XParamDB* findParamBySematic(const char* name) {

		const size_t num = sizeof(g_SematicParams) / sizeof(XParamDB);
		size_t i;

		for (i = 0; i < num; i++)
		{
			if (core::strUtil::IsEqualCaseInsen(name, g_SematicParams[i].name))
				return &g_SematicParams[i];
		}	
		return nullptr;
	}
}


XHWShader* XHWShader::forName(const char* shader_name, const char* entry,
	const char* sourceFile, const Flags<TechFlag>& techFlags,
	ShaderType::Enum type, Flags<ILFlag> ILFlags, uint32_t sourceCrc)
{
	X_ASSERT_NOT_NULL(s_pHWshaders);
	X_ASSERT_NOT_NULL(shader_name);
	X_ASSERT_NOT_NULL(entry);
	X_ASSERT_NOT_NULL(sourceFile);

	XHWShader_Dx10* pShader;
	core::StackString512 name;

	name.appendFmt("%s@%s", shader_name, entry);

	// macros are now part of the name.
	name.appendFmt("_%x", techFlags.ToInt());
	
	// input layout flags are also part of the name.
	name.appendFmt("_%x", ILFlags.ToInt());


#if X_DEBUG
	X_LOG1("Shader", "HWS for name: \"%s\"", name.c_str());
#endif // !X_DEBUG

	pShader = static_cast<XHWShader_Dx10*>(s_pHWshaders->findAsset(name.c_str()));

	if (pShader)
	{
		pShader->addRef();

		if (pShader->sourceCrc32_ != sourceCrc)
		{
			// shieeet, the shader needs updating.
			// we have to relase the old one and set it up fresh.
			pShader->releaseHW(); 
			pShader->sourceCrc32_ = sourceCrc;
			pShader->setStatus(ShaderStatus::NotCompiled);

			// remove the cache file, to save a file load / crc check.
			core::Path<char> path;
			pShader->getShaderCompileDest(path);

			// delete it!
			gEnv->pFileSys->deleteFile(path.c_str());

			// temp
		//	pShader->activate();
		}
	}
	else
	{
		pShader = X_NEW_ALIGNED(XHWShader_Dx10,g_rendererArena,"HWShader", X_ALIGN_OF(XHWShader_Dx10));
		pShader->name_ = name.c_str();
		pShader->type_ = type;
		pShader->sourceFileName_ = sourceFile;
		pShader->entryPoint_ = entry;
		pShader->sourceCrc32_ = sourceCrc;

		// save macros
		pShader->techFlags_ = techFlags;

		// temp
	//	pShader->activate();

		// register it.
		s_pHWshaders->AddAsset(name.c_str(), pShader);
	}


	return pShader;
}


bool XHWShader::Compile(XHWShader* pShader)
{
	X_ASSERT_NOT_NULL(pShader);

	return static_cast<XHWShader_Dx10*>(pShader)->activate();
}


void XHWShader_Dx10::InitBufferPointers(void)
{
	// already allocated?
	if (pConstBuffers_[0][0] != nullptr)
		return;

	int i, x;

	for (i = 0; i < ConstbufType::Num; i++)
	{
		for (x = 0; x < ShaderType::ENUM_COUNT; x++)
		{
			size_t size = 256;

			pConstBuffers_[x][i] = X_NEW_ARRAY_ALIGNED(ID3D11Buffer*,size,g_rendererArena,"ShaderConstBuffers",X_ALIGN_OF(ID3D11Buffer*));
			memset(pConstBuffers_[x][i], 0, sizeof(ID3D11Buffer*)*(size));
		}
	}


}

void XHWShader_Dx10::FreeBufferPointers(void)
{
	int i, x;

	for (i = 0; i < ConstbufType::Num; i++)
	{
		for (x = 0; x < ShaderType::ENUM_COUNT; x++)
		{
			X_DELETE_ARRAY(pConstBuffers_[x][i], g_rendererArena);
		}
	}

}

void XHWShader_Dx10::FreeHWShaders(void)
{
	// null on start up failed.
	X_ASSERT_NOT_NULL(s_pHWshaders);

	// NOTE: there should be none, since shaders that use them should release them.
	// so if there are any here there is a problem.
	// but we still clean up like a good boy.
	if(s_pHWshaders)
	{
		core::XResourceContainer::ResourceItor it = s_pHWshaders->begin();
		XHWShader_Dx10* pShader;

		for (; it != s_pHWshaders->end();)
		{
			pShader = static_cast<XHWShader_Dx10*>(it->second);
			++it;

			if (!pShader)
				continue;

			X_WARNING("HWShaders", "\"%s\" was not deleted", pShader->name_.c_str());

			pShader->release();
		}
	}
}


void XHWShader_Dx10::FreeParams(void)
{
	int i;
	for (i = 0; i < ShaderType::ENUM_COUNT; i++) {
		perFrameParams_[i].free();
		perInstantParams_[i].free();
	}
}




// ------------------------------------------------------------------

XHWShader_Dx10::XHWShader_Dx10() :
	pBlob_(nullptr),
	pHWHandle_(nullptr),
	bindVars_(g_rendererArena)
{
	core::zero_object(maxVecs_);

}

void XHWShader_Dx10::getShaderCompilePaths(core::Path<char>& src, core::Path<char>& dest)
{
	src.clear();
	src.appendFmt("shaders/temp/%s.merged", sourceFileName_.c_str());

	dest.clear();
	dest.appendFmt("shaders/compiled/%s.fxcb", name_.c_str());

	// make sure the directory is created.
	gEnv->pFileSys->createDirectoryTree(src.c_str());
	gEnv->pFileSys->createDirectoryTree(dest.c_str());
}

void XHWShader_Dx10::getShaderCompileSrc(core::Path<char>& src)
{
	src.clear();
	src.appendFmt("shaders/temp/%s.fxcb", name_.c_str());

	// make sure the directory is created.
	gEnv->pFileSys->createDirectoryTree(src.c_str());
}


void XHWShader_Dx10::getShaderCompileDest(core::Path<char>& dest)
{
	dest.clear();
	dest.appendFmt("shaders/compiled/%s.fxcb", name_.c_str());

	// make sure the directory is created.
	gEnv->pFileSys->createDirectoryTree(dest.c_str());
}


bool XHWShader_Dx10::saveToCache(void)
{
	core::Path<char> dest;
	getShaderCompileDest(dest);

	// write the compiled version.
	if (bin_.saveShader(dest.c_str(), sourceCrc32_, this))
	{
		X_LOG1("Shader", "saved shader to cache file: \"%s\"", dest.c_str());
	}
	else
	{
		X_ERROR("Shader", "failed to save shader to cache");
		return false;
	}

	return true;
}


bool XHWShader_Dx10::loadFromCache(void)
{
	core::Path<char> dest;

	getShaderCompileDest(dest);

	// we should check if a compiled version already exsists!
	if (bin_.loadShader(dest.c_str(), sourceCrc32_, this))
	{
		X_LOG0("Shader", "shader loaded from cache: \"%s\"", name_.c_str());

		// add them.
		addGlobalParams(bindVars_, this->type_);

		return true;
	}

	return false;
}

bool XHWShader_Dx10::loadFromSource(void)
{
	core::string source;

	XShaderManager* pShaderMan = &render::gRenDev->ShaderMan_;

	// we need to get the whole file :D
	if (!pShaderMan->sourceToString(source, this->sourceFileName_))
	{
		X_ERROR("Shader", "failed to get source for compiling");
		return false;
	}

	// save copy of merged shader for debugging.
	if(writeMergedSource_ == 1)
	{
		core::Path<char> src;
		getShaderCompileSrc(src);

		src /= ".hlsl";

		core::XFileScoped fileOut;
		if (fileOut.openFile(src.c_str(), core::fileModeFlags::RECREATE |
			core::fileModeFlags::WRITE))
		{
			fileOut.write(source.data(), source.length());
		}
	}

	if (asyncShaderCompile_)
	{
		source_ = source;

		core::V2::JobSystem* pJobSys = gEnv->pJobSys;
		core::V2::Job* pJob = pJobSys->CreateMemberJob<XHWShader_Dx10>(this,
			&XHWShader_Dx10::CompileShader_job, nullptr);
		pJobSys->Run(pJob);

		status_ = ShaderStatus::Compiling;
		return true;
	}

	return compileFromSource(source);
}


bool XHWShader_Dx10::compileFromSource(core::string& source)
{
	HRESULT hr;
	XShaderManager* pShaderMan;

	X_LOG0("Shader", "Compiling shader: \"%s\"", name_.c_str());

	pShaderMan = &render::gRenDev->ShaderMan_;
	D3DCompileflags_ = 0;

#if X_DEBUG // todo make this a cvar
	D3DCompileflags_ |= D3DCOMPILE_OPTIMIZATION_LEVEL0 | D3DCOMPILE_DEBUG;
#endif // !X_DEBUG

	// allow 16 flags.
	D3D_SHADER_MACRO Shader_Macros[17] = { NULL };
	core::string names[16];
	
	// i turn all set flags into strings.
	if (techFlags_.IsAnySet())
	{
		uint32_t numFlags = core::bitUtil::CountBits(techFlags_.ToInt());
		uint32_t macroIdx = 0;

		for (uint32_t i = 1; i < TechFlag::FLAGS_COUNT; i++)
		{
			uint32_t flag = (1 << i);
			if (techFlags_.IsSet((TechFlag::Enum)flag))
			{
				// we "X_" prefix and upper case.
				core::string& name = names[macroIdx];
				name = "X_";
				name += TechFlag::ToString(flag);
				name.toUpper();
				// set the pointer.
				Shader_Macros[macroIdx].Name = name.c_str();
				Shader_Macros[macroIdx].Definition = "1";
				macroIdx++;
			}
		}

		// log the macros
		for (size_t i = 0; i < numFlags; i++)
		{
			X_LOG0("Shader", "Macro(%" PRIuS "): name: \"%s\" value: \"%s\"",
				i, Shader_Macros[i].Name, Shader_Macros[i].Definition);
		}
	}

	ID3DBlob* error;

	core::StopWatch timer;

	core::string sourcName = name_ + ".fxcb.hlsl";	

	hr = D3DCompile(
		source,
		source.length(),
		sourcName,
		Shader_Macros, // pDefines
		NULL, // pInclude
		this->entryPoint_,
		getProfileFromType(type_),
		D3DCompileflags_, // Flags
		0, // Flags2
		&pBlob_,
		&error
		);

	if (FAILED(hr) || error || !pBlob_)
	{
		if (error)
		{
			const char* err = static_cast<const char*>(error->GetBufferPointer());

			core::StackString<4096> filterd(err, err + strlen(err));

			// skip file path.
			err = filterd.find(this->sourceFileName_);

			if (err) {
				core::StackString512 path(filterd.begin(), err);
				filterd.replaceAll(path.c_str(), "");
			}

			X_ERROR("Shader", "Failed to compile(%x): %s", hr, filterd.c_str());
		}
		else
		{
			X_ERROR("Shader", "Failed to compile: %x", hr);

		}

		return false;
	}

	float elapsed = timer.GetMilliSeconds();

	X_LOG0("Shader", "Compile complete: %.3fms", elapsed);

	return true;
}

void XHWShader_Dx10::CompileShader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	if (!compileFromSource(source_)) {
		status_ = ShaderStatus::FailedToCompile;
	}
	else {
		status_ = ShaderStatus::AsyncCompileDone;
	}

	source_.clear();
}

bool XHWShader_Dx10::uploadtoHW(void)
{
	HRESULT hr;
	ID3D11Device* pDevice;
	
	const void* pBuf = pBlob_->GetBufferPointer();
	uint32 nSize = safe_static_cast<uint32_t,size_t>(pBlob_->GetBufferSize());

	pDevice = render::g_Dx11D3D.DxDevice();

	if (this->type_ == ShaderType::Vertex)
		hr = pDevice->CreateVertexShader(pBuf, nSize, NULL, reinterpret_cast<ID3D11VertexShader**>(&pHWHandle_));
	else if (this->type_ == ShaderType::Pixel)
		hr = pDevice->CreatePixelShader(pBuf, nSize, NULL, reinterpret_cast<ID3D11PixelShader**>(&pHWHandle_));
	else if (this->type_ == ShaderType::Geometry)
		hr = pDevice->CreateGeometryShader(pBuf, nSize, NULL, reinterpret_cast<ID3D11GeometryShader**>(&pHWHandle_));
	else
	{
		// O'Deer
		hr = 0; // prevent warning 4701
		X_ASSERT_UNREACHABLE();
	}

	if (SUCCEEDED(hr)) 
	{
		// compiled out in debug
		if (this->type_ == ShaderType::Vertex)
			render::D3DDebug::SetDebugObjectName(reinterpret_cast<ID3D11VertexShader*>(pHWHandle_), this->entryPoint_);
		else if (this->type_ == ShaderType::Pixel)
			render::D3DDebug::SetDebugObjectName(reinterpret_cast<ID3D11PixelShader*>(pHWHandle_), this->entryPoint_);
		else if (this->type_ == ShaderType::Geometry)
			render::D3DDebug::SetDebugObjectName(reinterpret_cast<ID3D11GeometryShader*>(pHWHandle_), this->entryPoint_);
		// ~

		status_ = ShaderStatus::UploadedToHW;
		return true;
	}
	return  false;
}


Flags<ParamFlags> VarTypeToFlags(const D3D11_SHADER_TYPE_DESC& CDesc)
{
	Flags<ParamFlags> f;

	if (CDesc.Class == D3D10_SVC_MATRIX_COLUMNS)
	{
		f.Set(ParamFlags::MATRIX);
	}
	else if (CDesc.Class == D3D10_SVC_VECTOR)
	{
		if(CDesc.Columns == 4)
			f.Set(ParamFlags::VEC4);
		else if (CDesc.Columns == 3)
			f.Set(ParamFlags::VEC3);
		else if (CDesc.Columns == 2)
			f.Set(ParamFlags::VEC2);
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}
	}
	else
	{
		X_ASSERT_NOT_IMPLEMENTED();
	}

	return f;
}

bool XHWShader_Dx10::reflectShader(void)
{
	ID3D11Device* pDevice;
	ID3D11ShaderReflection* pShaderReflection;
	ID3D11ShaderReflectionConstantBuffer* pCB;
	D3D11_SHADER_DESC shaderDesc;

	pDevice = render::g_Dx11D3D.DxDevice();

	const void* pBuf = pBlob_->GetBufferPointer();
	size_t nSize = pBlob_->GetBufferSize();
	uint32 i, n;

	core::Array<XShaderParam> BindVars(g_rendererArena);

	if (FAILED(D3DReflect(pBuf, nSize, IID_ID3D11ShaderReflection, (void **)&pShaderReflection)))
	{
		X_ERROR("Shader", "failed to reflect shader: %s", name_.c_str());
		return false;
	}

	pShaderReflection->GetDesc(&shaderDesc);

	X_LOG_BULLET;
	X_LOG0("Shader", "Instructions: %i", shaderDesc.InstructionCount);
	X_LOG0("Shader", "ConstantBuffers: %i", shaderDesc.ConstantBuffers);
	X_LOG0("Shader", "BoundResources: %i", shaderDesc.BoundResources);
	X_LOG0("Shader", "InputParameters: %i", shaderDesc.InputParameters);
	X_LOG0("Shader", "OutputParameters: %i", shaderDesc.OutputParameters);

	for (n = 0; n<shaderDesc.ConstantBuffers; n++)
	{
		pCB = pShaderReflection->GetConstantBufferByIndex(n);

		D3D11_SHADER_BUFFER_DESC BufferDesc;
		pCB->GetDesc(&BufferDesc);

		if (BufferDesc.Type == D3D11_CT_RESOURCE_BIND_INFO)
			continue;

		ConstbufType::Enum constBuftype = ConstbufType::Num;

		if (core::strUtil::IsEqual("$Globals", BufferDesc.Name))
			constBuftype = ConstbufType::PER_BATCH;
		else if (core::strUtil::FindCaseInsensitive("PerFrameConstants", BufferDesc.Name))
			constBuftype = ConstbufType::PER_FRAME;
		else if (core::strUtil::FindCaseInsensitive("ObjectConstants", BufferDesc.Name))
			constBuftype = ConstbufType::PER_INSTANCE;

		if (constBuftype == ConstbufType::Num) {
			X_WARNING("Shader", "Unknown cbuffer name, ignoring.");
			continue;
		}

		for (i = 0; i<BufferDesc.Variables; i++)
		{
			ID3D11ShaderReflectionVariable* pCV = pCB->GetVariableByIndex(i);
			ID3D11ShaderReflectionType* pVT = pCV->GetType();
			D3D11_SHADER_VARIABLE_DESC CDesc;
			D3D11_SHADER_TYPE_DESC CTDesc;
			pVT->GetDesc(&CTDesc);
			pCV->GetDesc(&CDesc);
			
			int nReg = CDesc.StartOffset >> 4;

			// used and abused?
			if (!core::bitUtil::IsBitFlagSet(CDesc.uFlags, D3D10_SVF_USED))
				continue;

			XParamDB* entry = findParamBySematic(CDesc.Name);

			if (!entry) 
			{
			//	X_ERROR("Shader", "unknown input var ignoring: \"%s\"", CDesc.Name);
				X_WARNING("Shader", "unknown input var must be set manualy: \"%s\"", CDesc.Name);

				// if we are here the var is used, so the shader won't work correct.
				// no point letting it run with a invalid buffer.
			//	pShaderReflection->Release();
			//	return false;

				// going allow it to be added, then it can be set manualy via code.
				XShaderParam bind;

				bind.name = CDesc.Name;
				bind.nameHash = core::StrHash(CDesc.Name);
				bind.bind = safe_static_cast<short, int>(nReg);
				bind.constBufferSlot = safe_static_cast<short, ConstbufType::Enum>(constBuftype);
				bind.numParameters = (CDesc.Size + 15) >> 4; // vec4
				bind.flags = VarTypeToFlags(CTDesc);
				bind.type = ParamType::Unknown;

				BindVars.push_back(bind);
				continue;
			}

			if (entry->flags.IsSet(ParamFlags::MATRIX))
			{
				if (CTDesc.Class != D3D10_SVC_MATRIX_COLUMNS && CTDesc.Class != D3D10_SVC_MATRIX_ROWS)
				{
					X_ERROR("Shader", "input var: \"%s\" should be a matrix", CDesc.Name);
					return false;
				}
				if (CTDesc.Rows != 4 || CTDesc.Columns != 4)
				{
					X_ERROR("Shader", "input var: \"%s\" should be a 4x4 matrix", CDesc.Name);
					return false;
				}

			}
			else if (entry->flags.IsSet(ParamFlags::VEC4))
			{
				if (CTDesc.Class != D3D10_SVC_VECTOR || CTDesc.Columns != 4)
				{
					X_ERROR("Shader", "input var: \"%s\" should be a float4", CDesc.Name);
					return false;
				}
			}
			else if (entry->flags.IsSet(ParamFlags::FLOAT))
			{
				if (CTDesc.Class != D3D10_SVC_SCALAR || CTDesc.Columns != 1 || CTDesc.Rows != 1)
				{
					X_ERROR("Shader", "input var: \"%s\" should be a float(1)", CDesc.Name);
					return false;
				}
			}
			else
			{
				X_ASSERT_NOT_IMPLEMENTED();
				return false;
			}


			XShaderParam bind;
			bind.name = CDesc.Name;
			bind.nameHash = core::StrHash(CDesc.Name);
			bind.bind = safe_static_cast<short, int>(nReg);
			bind.constBufferSlot = safe_static_cast<short, ConstbufType::Enum>(constBuftype);
			bind.numParameters = (CDesc.Size + 15) >> 4; // vec4
			bind.flags = entry->flags;
			bind.type = entry->type;
			
			BindVars.push_back(bind);
		}
	}

	D3D11_SHADER_INPUT_BIND_DESC InputBindDesc;
	for (n = 0; n<shaderDesc.BoundResources; n++)
	{
		core::zero_object(InputBindDesc);
		pShaderReflection->GetResourceBindingDesc(n, &InputBindDesc);

		switch (InputBindDesc.Type)
		{
			case D3D_SIT_CBUFFER:
			break;

			case D3D_SIT_TBUFFER:
			break;

			case D3D_SIT_TEXTURE:
			break;

			case D3D_SIT_SAMPLER:
			break;

			default:
				break;
		}

		if (InputBindDesc.Type != D3D10_SIT_TEXTURE)
			continue;

		XShaderParam bind;

		bind.name = InputBindDesc.Name;
		bind.bind = safe_static_cast<short, UINT>(InputBindDesc.BindPoint | SHADER_BIND_SAMPLER);
		bind.numParameters = InputBindDesc.BindCount;
	//	BindVars.push_back(bind);
	}

	for (i = 0; i < BindVars.size(); i++)
	{
		XShaderParam* pB = &BindVars[i];
		if (!(pB->bind & SHADER_BIND_SAMPLER))
			continue;

		numSamplers_++;

		// find it's sampler
		for (n = 0; n < shaderDesc.BoundResources; n++)
		{
			core::zero_object(InputBindDesc);

			pShaderReflection->GetResourceBindingDesc(n, &InputBindDesc);

			if (InputBindDesc.Type != D3D10_SIT_SAMPLER)
				continue;

			core::StackString512 temp(InputBindDesc.Name);

			// if the sampler state has the same name
			// as the texture link them.
			if (temp.isEqual(pB->name))
			{
				pB->constBufferSlot = safe_static_cast<short,UINT>(InputBindDesc.BindPoint);
				break;
			}

			// nameing convention: (name + sampler)
			// Texture	basemap
			// samnpler basemapSampler
			if (temp.findCaseInsen(pB->name) && temp.findCaseInsen("sampler"))
			{
				pB->constBufferSlot = safe_static_cast<short, UINT>(InputBindDesc.BindPoint);
				break;
			}
		}
	}

	for (i = 0; i < BindVars.size(); i++)
	{
		XShaderParam* pB = &BindVars[i];
		 // const char *param = pB->name.c_str();
		bool bSampler = (pB->bind & SHADER_BIND_SAMPLER) != 0;

		if (!bSampler)
		{
			X_LOG0("Shader", "add FX Parameter: \"%s\", %i, Cb:%i (%i)", pB->name.c_str(), 
				pB->numParameters, pB->constBufferSlot, pB->bind );

		}
		else
		{
			X_LOG0("Shader", "add sampler: \"%s\", %i", pB->name.c_str(), pB->bind);
		}

		// set max slots
		if (pB->constBufferSlot < 3)
			maxVecs_[pB->constBufferSlot] = core::Max(pB->bind + pB->numParameters, maxVecs_[pB->constBufferSlot]);
	}

	if (this->type_ == ShaderType::Vertex)
	{
		const ILTreeNode* pILnode = &this->ILTree_;
		D3D11_SIGNATURE_PARAMETER_DESC InputDsc;
		for (n = 0; n < shaderDesc.InputParameters; n++)
		{
			pShaderReflection->GetInputParameterDesc(n, &InputDsc);

			// how many?
			// uint32_t numVars = core::bitUtil::CountBits<uint32_t>(InputDsc.Mask);
			// i only do sematic checks now, since i don't give a flying fuck about the format.
			pILnode = pILnode->GetChildWithSemtaic(InputDsc.SemanticName);
			if (!pILnode)
			{
				X_ERROR("Shader", "input layout invalid.");
				return true;
			}
		}

		X_ASSERT_NOT_NULL(pILnode);

		InputLayoutFormat::Enum fmt = pILnode->GetILFmt();

		X_ASSERT(fmt != InputLayoutFormat::Invalid, "failed to detect correct input layout format")(fmt);
		// work out the format from the node.
		X_LOG0("Shader", "InputLayout Fmt: \"%s\"", InputLayoutFormat::ToString(fmt));

		IlFmt_ = fmt;
	}
	else if (type_ == ShaderType::Pixel)
	{
		D3D11_SIGNATURE_PARAMETER_DESC OutputDsc;

		for (n = 0; n < shaderDesc.OutputParameters; n++)
		{
			pShaderReflection->GetOutputParameterDesc(n, &OutputDsc);

		}
	}


	X_LOG_BULLET;
	for (i = 0; i < BindVars.size(); i++)
	{
		X_LOG0("ShaderPram", "Name: \"%s\"", BindVars[i].name.c_str());
	}

	pShaderReflection->Release();

	// add them.
	addGlobalParams(BindVars, this->type_);

	// shader has a copy!
	bindVars_ = BindVars;

	// save some data.
	if (type_ == ShaderType::Pixel) {
		numRenderTargets_ = shaderDesc.OutputParameters;
	}
	numConstBuffers_ = shaderDesc.ConstantBuffers;
	numInputParams_ = shaderDesc.InputParameters;

	return true;
}



bool XHWShader_Dx10::activate(void)
{
	if (!isValid())
	{
		if (isCompiling())
		{
			if (status_ == ShaderStatus::AsyncCompileDone)
			{
				if (uploadtoHW())
				{
					if (reflectShader())
					{
						status_ = ShaderStatus::ReadyToRock;
						saveToCache();
						return true;
					}
				}
			}

			return false;
		}

		if (FailedtoCompile())
			return false;

		if (loadFromCache())
		{
			if (uploadtoHW())
			{
				// reflection not required for cache loaded.
				status_ = ShaderStatus::ReadyToRock;
				return true;
			}
			else
			{
				// fall throught and load from source.
			}
		}

		if (!loadFromSource())
		{
			// we need to set this shader as broken.
			// instead of trying to compile it all the time.
			status_ = ShaderStatus::FailedToCompile;
			X_LOG0("Shader", "Failed to activate shader: \"%s\"", getName());
			return false;
		}

		if (isCompiling()) {
			X_LOG0("Shader", "shader: \"%s\" is compiling", getName());
			return false;
		}

		if (uploadtoHW())
		{
			if (reflectShader())
			{
				status_ = ShaderStatus::ReadyToRock;
				saveToCache();
				return true;
			}
		}

		X_LOG0_EVERY_N(10, "Shader", "Failed to activate shader: \"%s\"", getName());
		return false;
	}
	return true;
}


const int XHWShader_Dx10::release(void)
{
	int res = XHWShader::release();
	if (res)
		return res;

	releaseHW();

	X_DELETE(this,g_rendererArena);

	return 0;
}

const int XHWShader_Dx10::releaseHW(void)
{
	ShaderStatus::Enum status = this->status_;
	ShaderType::Enum type = this->type_;
	void* pHandle = pHWHandle_;

	if (status == ShaderStatus::ReadyToRock)
	{
		if (type == ShaderType::Vertex)
		{
			if (pCurVS_ == this)
				pCurVS_ = nullptr;
			return ((ID3D11VertexShader*)pHandle)->Release();
		}
		else if (type == ShaderType::Pixel)
		{
			if (pCurPS_ == this)
				pCurPS_ = nullptr;
			return ((ID3D11PixelShader*)pHandle)->Release();
		}
		else if (type == ShaderType::Geometry)
		{
			if (pCurGS_ == this)
				pCurGS_ = nullptr;
			return ((ID3D11GeometryShader*)pHandle)->Release();
		}
		else
		{
			// O'Deer
			X_ASSERT_UNREACHABLE();
		}
	}
	return 0;
}

// ===================================================

bool XHWShader_Dx10::bindVS(void)
{
	// make sure the shader is loaded
	// and compiled / ready for binding.
	if (!activate())
		return false;


	// no point changing if it's the same.
	if (pCurVS_ != this) {
		pCurVS_ = this;
		setShader();
	}
	return true;
}

bool XHWShader_Dx10::bindPS(void)
{
	if (!activate())
		return false;


	if (pCurPS_ != this) {
		pCurPS_ = this;
		setShader();
	}
	return true;
}

bool XHWShader_Dx10::bindGS(void)
{
	if (!activate())
		return false;


	if (pCurGS_ != this) {
		pCurGS_ = this;
		setShader();
	}
	return true;
}

XShaderParam* XHWShader_Dx10::getParameter(const core::StrHash& nameHash)
{
	core::Array<XShaderParam>::size_type i;
	for (i=0; i < bindVars_.size(); i++)
	{
		if (bindVars_[i].nameHash == nameHash)
			return &bindVars_[i];
	}

	return nullptr;
}

// ====================== Static =============================


void XHWShader_Dx10::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pConsole);
	X_ASSERT_NOT_NULL(g_rendererArena);

	s_pHWshaders = X_NEW_ALIGNED(render::XRenderResourceContainer, g_rendererArena,
		"HwShaderRes", X_ALIGN_OF(render::XRenderResourceContainer))(g_rendererArena, 256);


	int i;
	for (i = 0; i < ShaderType::ENUM_COUNT; i++) {
		perFrameParams_[i].setArena(g_rendererArena);
		perInstantParams_[i].setArena(g_rendererArena);	
	}

	InitBufferPointers();
	CreateInputLayoutTree();
	RegisterDvars();
}

void XHWShader_Dx10::RegisterDvars(void)
{
	ADD_CVAR_REF("shader_writeMergedSource", writeMergedSource_, 1, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, 
		"Writes the merged shader source to file before it's compiled");
	ADD_CVAR_REF("shader_asyncCompile", asyncShaderCompile_, 1, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Performs hardware shader compiling async");
}

void XHWShader_Dx10::shutDown(void)
{
	X_LOG0("HWShaders", "Shutting Down");

	FreeBufferPointers();
	FreeHWShaders();
	FreeParams();
	FreeInputLayoutTree();

	if (s_pHWshaders) {
		core::Mem::DeleteAndNull(s_pHWshaders, g_rendererArena);
	}
}

void XHWShader_Dx10::bindGS(XHWShader_Dx10* pShader)
{
	ID3D11DeviceContext* pDevice = render::g_Dx11D3D.DxDeviceContext();
	// pShader can be null (disables geo shader)
	if (pShader != pCurGS_) {
		pCurGS_ = pShader;
		pDevice->GSSetShader(pShader ? ((ID3D11GeometryShader*)pShader->pHWHandle_) : nullptr, nullptr, 0);
	}
}


void XHWShader_Dx10::commitConstBuffer(ConstbufType::Enum buf_type, ShaderType::Enum shader_type)
{
	// unmap it :)
	// it checks if any updates where made.
	unMapConstbuffer(shader_type, buf_type);
}

void XHWShader_Dx10::comitParamsGlobal(void)
{
	X_PROFILE_BEGIN("setParamsGlobal", core::ProfileSubSys::RENDER);

	setParamsGlobal();

	commitConstBuffer(ConstbufType::PER_FRAME, ShaderType::Vertex);
	commitConstBuffer(ConstbufType::PER_FRAME, ShaderType::Pixel);
}

void XHWShader_Dx10::comitParams(void)
{
	X_PROFILE_BEGIN("setParams", core::ProfileSubSys::RENDER);

//	setParams();

	commitConstBuffer(ConstbufType::PER_BATCH, ShaderType::Vertex);
	commitConstBuffer(ConstbufType::PER_BATCH, ShaderType::Pixel);

	commitConstBuffer(ConstbufType::PER_INSTANCE, ShaderType::Vertex);
	commitConstBuffer(ConstbufType::PER_INSTANCE, ShaderType::Pixel);

	// Geo shader
	if (pCurGS_) {
		commitConstBuffer(ConstbufType::PER_BATCH, ShaderType::Geometry);
		commitConstBuffer(ConstbufType::PER_INSTANCE, ShaderType::Geometry);
	}

}



void XHWShader_Dx10::setParamsGlobal(void)
{
	if (!perFrameParams_[ShaderType::Vertex].isEmpty())
	{
		setParamValues(
			perFrameParams_[ShaderType::Vertex].ptr(),
			(uint32_t)perFrameParams_[ShaderType::Vertex].size(),
			ShaderType::Vertex, 
			perFrameMaxVecs_[ShaderType::Vertex]
		);
	}
	if (!perFrameParams_[ShaderType::Pixel].isEmpty())
	{
		setParamValues(
			perFrameParams_[ShaderType::Pixel].ptr(),
			(uint32_t)perFrameParams_[ShaderType::Pixel].size(),
			ShaderType::Pixel,
			perFrameMaxVecs_[ShaderType::Pixel]
		);
	}

}

void XHWShader_Dx10::setParams(void)
{
	// we basically just loop over the parama update the values, 
	// and copy them into the mapped buffer.

	if (!perInstantParams_[ShaderType::Vertex].isEmpty())
	{
		setParamValues(
			perInstantParams_[ShaderType::Vertex].ptr(),
			(uint32_t)perInstantParams_[ShaderType::Vertex].size(),
			ShaderType::Vertex,
			perInstantMaxVecs_[ShaderType::Vertex]
			);
	}
	if (!perInstantParams_[ShaderType::Pixel].isEmpty())
	{
		setParamValues(
			perInstantParams_[ShaderType::Pixel].ptr(),
			(uint32_t)perInstantParams_[ShaderType::Pixel].size(),
			ShaderType::Pixel,
			perInstantMaxVecs_[ShaderType::Pixel]
		);
	}


}


void XHWShader_Dx10::setParamValues(XShaderParam* pPrams, uint32_t numPrams,
	ShaderType::Enum shaderType, uint32_t maxVecs)
{
	X_ASSERT_NOT_NULL(pPrams);

	uint32_t i, num;
	XShaderParam* pCurParam;
	Vec4f* pDest;

	num = numPrams;
	pDest = vecTemp;

	render::DX11XRender* const __restrict r = &render::g_Dx11D3D;

//	XHWShader_Dx10::InitBufferPointers();

	for (i = 0; i < num; i++)
	{
		pCurParam = &pPrams[i];

		switch (pCurParam->type)
		{
			case ParamType::PF_worldToCameraMatrix:
				getworldToCameraMatrix(r);
				break;
			case ParamType::PF_worldToScreenMatrix:
				getworldToScreenMatrix(r);
				break;
			case ParamType::PF_cameraToWorldMatrix:
				getcameraToWorldMatrix(r);
				break;

			case ParamType::PI_objectToWorldMatrix:
				getObjectToWorldMatrix(r);
				break;

			case ParamType::PI_worldMatrix:
				getWorldMatrix(r);
				break;
			case ParamType::PI_viewMatrix:
				getviewMatrix(r);
				break;
			case ParamType::PI_projectionMatrix:
				getProjectionMatrix(r);
				break;
			case ParamType::PI_worldViewProjectionMatrix:
				getWorldViewProjectionMatrix(r);
				break;


			case ParamType::PF_Time:
				getTime(r);
				break;
			case ParamType::PF_FrameTime:
				getFrameTime(r);
				break;
			case ParamType::PF_CameraPos:
				getCameraPos(r);
				break;
			case ParamType::PF_ScreenSize:
				getScreenSize(r);
				break;

				// Per Batch
			case ParamType::PB_CameraFront:
				getCameraFront(r);
				break;
			case ParamType::PB_CameraRight:
				getCameraRight(r);
				break;
			case ParamType::PB_CameraUp:
				getCameraUp(r);
				break;

#if X_DEBUG
			default:
//				X_ASSERT_UNREACHABLE();
				break;
#else
				X_NO_SWITCH_DEFAULT;
#endif
		}


		if (pCurParam->flags.IsSet(ParamFlags::INT))
			setParameteri(pCurParam, pDest, shaderType, maxVecs);
		else
			setParameterf(pCurParam, pDest, shaderType, maxVecs);
	}

}



// ~====================== Static =============================


void XHWShader_Dx10::CreateInputLayoutTree(void)
{
	// all the posible node types.
	ILTreeNode blank;
	ILTreeNode pos("POSITION");
	ILTreeNode uv("TEXCOORD");
	ILTreeNode col("COLOR");
	ILTreeNode nor("NORMAL");
	ILTreeNode tan("TANGENT");
	ILTreeNode bin("BINORMAL");
	
	// for shader input layouts the format is not given since the shader
	// don't care what the format comes in as.
	// so how can i work out what the formats are since i support identical sematic layouts
	// with diffrent foramts :(
	//
	// maybe i should just have a sematic format, which can be used to tell if the current input
	// layout will work with the shader :)
	//
	//        .
	//        |
	//       P3F_____
	//       / \     \
	//     T2S  T4F  T3F
	//      |    |__
	//     C4B	    |
	//	  __|	   C4B 
	//	 /  |       |
	// N3F N10	   N3F
	//  |    \
	// TB3F  TB10
	//

	ILTreeNode& uvBase = blank.AddChild(pos).AddChild(uv, InputLayoutFormat::POS_UV);
	uvBase.AddChild(col, InputLayoutFormat::POS_UV_COL).
		AddChild(nor, InputLayoutFormat::POS_UV_COL_NORM).
		AddChild(tan, InputLayoutFormat::POS_UV_COL_NORM_TAN).
		AddChild(bin, InputLayoutFormat::POS_UV_COL_NORM_TAN_BI);

	// double text coords.
	uvBase.AddChild(uv).
		AddChild(col).
		AddChild(nor, InputLayoutFormat::POS_UV2_COL_NORM);

	ILTree_ = blank;
}


void XHWShader_Dx10::FreeInputLayoutTree(void)
{
	// need to kill this.

	ILTree_.free();
}


// pILTree_

void XHWShader_Dx10::addGlobalParams(core::Array<XShaderParam>& BindVars, ShaderType::Enum type)
{
	core::Array<XShaderParam>::Iterator It, it2;
	core::Array<XShaderParam>& current = perFrameParams_[type];
	core::Array<XShaderParam>& currentPI = perInstantParams_[type];

	// I don't know how cbuffers will work across multipel shaders.
	// umm have to see how it works tbh.
	// like should i check name and register?
	// if register diffrent add it ? 

	// alright I played around with shit.
	// it seams Directx gives same register for identical names.

	It = BindVars.begin();
	for (; It != BindVars.end(); ++It)
	{
		const XShaderParam* param = It;

		it2 = current.begin();
		for (; it2 != current.end(); ++it2)
		{
			if (param->name == it2->name)
			{
				goto skip;
			}
		}

		if (param->constBufferSlot == shader::ConstbufType::PER_FRAME)
		{
			current.append(*param);
			perFrameMaxVecs_[type] = core::Max(param->bind + param->numParameters, perFrameMaxVecs_[type]);
		}
		else if (param->constBufferSlot == shader::ConstbufType::PER_INSTANCE)
		{	
			currentPI.append(*param);
			perInstantMaxVecs_[type] = core::Max(param->bind + param->numParameters, perInstantMaxVecs_[type]);
		}

	skip:
		;
	}
}



X_NAMESPACE_END