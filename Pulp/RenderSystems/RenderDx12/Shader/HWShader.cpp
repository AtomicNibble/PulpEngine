#include "stdafx.h"

#include "Shader.h"
#include "HWShader.h"
#include "ShaderManager.h"
#include "ShaderSourceTypes.h"

#include <IConsole.h>
#include <IFileSys.h>
#include <Threading\JobSystem2.h>

#include <D3Dcompiler.h>
#include <../../3rdparty/source/directx/D3DX9Shader.h>

#include <Time\StopWatch.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{

	namespace
	{

		Flags<ParamFlags> VarTypeToFlags(const D3D12_SHADER_TYPE_DESC& CDesc)
		{
			Flags<ParamFlags> f;

			if (CDesc.Class == D3D10_SVC_MATRIX_COLUMNS)
			{
				f.Set(ParamFlags::MATRIX);
			}
			else if (CDesc.Class == D3D10_SVC_VECTOR)
			{
				if (CDesc.Columns == 4) {
					f.Set(ParamFlags::VEC4);
				}
				else if (CDesc.Columns == 3) {
					f.Set(ParamFlags::VEC3);
				}
				else if (CDesc.Columns == 2) {
					f.Set(ParamFlags::VEC2);
				}
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

		XParamDB* findParamBySematic(const char* name)
		{
			const size_t num = sizeof(g_SematicParams) / sizeof(XParamDB);

			for (size_t i = 0; i < num; i++)
			{
				if (core::strUtil::IsEqualCaseInsen(name, g_SematicParams[i].name)) {
					return &g_SematicParams[i];
				}
			}
			return nullptr;
		}


		static const uint32_t SHADER_BIND_SAMPLER = 0x4000;

	} // namespace


	XShaderParam::XShaderParam() :
		type(ParamType::Unknown),
		bind(-2),
		constBufferSlot(0),
		numParameters(1)
	{

	}


	// -------------------------------------------------------------------

	XHWShader::XHWShader(core::MemoryArenaBase* arena, XShaderManager& shaderMan,
			ShaderType::Enum type, const char* pName, const core::string& entry,
			SourceFile* pSourceFile, TechFlags techFlags) :
		shaderMan_(shaderMan),
		name_(pName),
		entryPoint_(entry),
		status_(ShaderStatus::NotCompiled),
		type_(type),
		IlFmt_(InputLayoutFormat::POS_UV),
		numRenderTargets_(0),
		numSamplers_(0),
		numConstBuffers_(0),
		numInputParams_(0),
		bindVars_(arena),
		bytecode_(arena),
		D3DCompileflags_(0)
	{
		X_ASSERT_NOT_NULL(pSourceFile);

		sourceFileName_ = pSourceFile->getName();
		sourceCrc32_ = pSourceFile->getSourceCrc32();

		core::zero_object(maxVecs_);
	}


	bool XHWShader::compile(ShaderBin& shaderBin)
	{
		if (isValid()) {
			return true;
		}

		if (FailedtoCompile()) {
			return false;
		}

		if (loadFromCache(shaderBin)) {
			return true;
		}

		if (!loadFromSource()) {
			status_ = ShaderStatus::FailedToCompile;
			X_LOG0("Shader", "Failed to compile shader: \"%s\"", getName());
			return false;
		}

		return true;
	}

	bool XHWShader::invalidateIfChanged(uint32_t newSourceCrc32)
	{
		if (sourceCrc32_ != newSourceCrc32)
		{
			sourceCrc32_ = newSourceCrc32;

			// what todo if the status is compiling?
			// potentially we have a pending job that's about to run.
			// which would still use the old source.
			if (status_ == ShaderStatus::Compiling) {
				X_ASSERT_NOT_IMPLEMENTED();
			}

			status_ = ShaderStatus::NotCompiled;
			return true;
		}

		return false;
	}

	XShaderParam* XHWShader::getParameter(const core::StrHash& nameHash)
	{
		for (size_t i = 0; i < bindVars_.size(); i++)
		{
			if (bindVars_[i].nameHash == nameHash) {
				return &bindVars_[i];
			}
		}

		return nullptr;
	}


	void XHWShader::getShaderCompilePaths(core::Path<char>& src, core::Path<char>& dest)
	{
		src.clear();
		src.appendFmt("shaders/temp/%s.merged", sourceFileName_.c_str());

		dest.clear();
		dest.appendFmt("shaders/compiled/%s.fxcb", name_.c_str());

		// make sure the directory is created.
		gEnv->pFileSys->createDirectoryTree(src.c_str());
		gEnv->pFileSys->createDirectoryTree(dest.c_str());
	}

	void XHWShader::getShaderCompileSrc(core::Path<char>& src)
	{
		src.clear();
		src.appendFmt("shaders/temp/%s.fxcb", name_.c_str());

		// make sure the directory is created.
		gEnv->pFileSys->createDirectoryTree(src.c_str());
	}


	void XHWShader::getShaderCompileDest(core::Path<char>& dest)
	{
		dest.clear();
		dest.appendFmt("shaders/compiled/%s.fxcb", name_.c_str());

		// make sure the directory is created.
		gEnv->pFileSys->createDirectoryTree(dest.c_str());
	}


	bool XHWShader::loadFromCache(ShaderBin& shaderBin)
	{
		core::Path<char> dest;

		getShaderCompileDest(dest);

		// we should check if a compiled version already exsists!
		if (shaderBin.loadShader(dest.c_str(), this))
		{
			X_LOG0("Shader", "shader loaded from cache: \"%s\"", name_.c_str());
			return true;
		}

		return false;
	}

	bool XHWShader::saveToCache(ShaderBin& shaderBin)
	{
		core::Path<char> dest;
		getShaderCompileDest(dest);

		// write the compiled version.
		if (shaderBin.saveShader(dest.c_str(), this))
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

	bool XHWShader::loadFromSource(void)
	{
		core::string source;

		// we need to get the whole file :D
		if (!shaderMan_.sourceToString(source, this->sourceFileName_))
		{
			X_ERROR("Shader", "failed to get source for compiling");
			return false;
		}

		ShaderVars& vars = shaderMan_.getShaderVars();

		// save copy of merged shader for debugging.
		if (vars.writeMergedSource())
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

		if (vars.asyncCompile())
		{
			source_ = source;

			core::V2::JobSystem* pJobSys = gEnv->pJobSys;
			core::V2::Job* pJob = pJobSys->CreateMemberJob<XHWShader>(this, &XHWShader::CompileShader_job, nullptr);
			pJobSys->Run(pJob);

			status_ = ShaderStatus::Compiling;
			return true;
		}

		return compileFromSource(source);
	}


	bool XHWShader::compileFromSource(core::string& source)
	{
		X_LOG0("Shader", "Compiling shader: \"%s\"", name_.c_str());

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
				if (techFlags_.IsSet(static_cast<TechFlag::Enum>(flag)))
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

		ID3DBlob* pBlob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;

		core::StopWatch timer;

		core::string sourcName = name_ + ".fxcb.hlsl";

		HRESULT hr = D3DCompile(
			source,
			source.length(),
			sourcName,
			Shader_Macros, // pDefines
			NULL, // pInclude
			this->entryPoint_,
			getProfileFromType(type_),
			D3DCompileflags_, // Flags
			0, // Flags2
			&pBlob,
			&pErrorBlob
		);

		if (FAILED(hr) || pErrorBlob || !pBlob)
		{
			if (pErrorBlob)
			{
				const char* pErrStr = static_cast<const char*>(pErrorBlob->GetBufferPointer());

				core::StackString<4096> filterd(pErrStr, pErrStr + strlen(pErrStr));

				// skip file path.
				pErrStr = filterd.find(this->sourceFileName_);

				if (pErrStr) {
					core::StackString512 path(filterd.begin(), pErrStr);
					filterd.replaceAll(path.c_str(), "");
				}

				X_ERROR("Shader", "Failed to compile(%x): %s", hr, filterd.c_str());
			}
			else
			{
				X_ERROR("Shader", "Failed to compile: %x", hr);

			}

			core::SafeReleaseDX(pErrorBlob);
			core::SafeReleaseDX(pBlob);
			return false;
		}

		// copy the byte code.
		bytecode_.resize(pBlob->GetBufferSize());
		std::memcpy(bytecode_.data(), pBlob->GetBufferPointer(), pBlob->GetBufferSize());

		// release 
		core::SafeReleaseDX(pErrorBlob);
		core::SafeReleaseDX(pBlob);

		const float elapsed = timer.GetMilliSeconds();
		X_LOG0("Shader", "Compile complete: %.3fms", elapsed);

		return true;
	}

	void XHWShader::CompileShader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
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

	bool XHWShader::reflectShader(void)
	{
		ID3D12ShaderReflection* pShaderReflection;
		ID3D12ShaderReflectionConstantBuffer* pCB;
		D3D12_SHADER_DESC shaderDesc;
		uint32 i, n;

		core::Array<XShaderParam> BindVars(g_rendererArena);

		if (FAILED(D3DReflect(bytecode_.data(), bytecode_.size(), IID_ID3D12ShaderReflection, (void **)&pShaderReflection)))
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

			D3D12_SHADER_BUFFER_DESC BufferDesc;
			pCB->GetDesc(&BufferDesc);

			if (BufferDesc.Type == D3D11_CT_RESOURCE_BIND_INFO)
				continue;

			ConstbufType::Enum constBuftype;

			if (core::strUtil::IsEqual("$Globals", BufferDesc.Name)) {
				constBuftype = ConstbufType::PER_BATCH;
			}
			else if (core::strUtil::FindCaseInsensitive("PerFrameConstants", BufferDesc.Name)) {
				constBuftype = ConstbufType::PER_FRAME;
			}
			else if (core::strUtil::FindCaseInsensitive("ObjectConstants", BufferDesc.Name)) {
				constBuftype = ConstbufType::PER_INSTANCE;
			}
			else {
				X_WARNING("Shader", "Unknown cbuffer name, ignoring.");
				continue;
			}

			for (i = 0; i<BufferDesc.Variables; i++)
			{
				ID3D12ShaderReflectionVariable* pCV = pCB->GetVariableByIndex(i);
				ID3D12ShaderReflectionType* pVT = pCV->GetType();
				D3D12_SHADER_VARIABLE_DESC CDesc;
				D3D12_SHADER_TYPE_DESC CTDesc;
				pVT->GetDesc(&CTDesc);
				pCV->GetDesc(&CDesc);

				int nReg = CDesc.StartOffset >> 4;

				// used and abused?
				if (!core::bitUtil::IsBitFlagSet(CDesc.uFlags, D3D10_SVF_USED))
					continue;

				XParamDB* entry = findParamBySematic(CDesc.Name);

				if (!entry)
				{
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

		D3D12_SHADER_INPUT_BIND_DESC InputBindDesc;
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
			if (!(pB->bind & SHADER_BIND_SAMPLER)) {
				continue;
			}

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
					pB->constBufferSlot = safe_static_cast<short, UINT>(InputBindDesc.BindPoint);
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
					pB->numParameters, pB->constBufferSlot, pB->bind);

			}
			else
			{
				X_LOG0("Shader", "add sampler: \"%s\", %i", pB->name.c_str(), pB->bind);
			}

			// set max slots
			if (pB->constBufferSlot < 3) {
				maxVecs_[pB->constBufferSlot] = core::Max(pB->bind + pB->numParameters, maxVecs_[pB->constBufferSlot]);
			}
		}

		if (this->type_ == ShaderType::Vertex)
		{
			const ILTreeNode* pILnode = &shaderMan_.getILTree();
			D3D12_SIGNATURE_PARAMETER_DESC InputDsc;
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
			D3D12_SIGNATURE_PARAMETER_DESC OutputDsc;

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
		// addGlobalParams(BindVars, this->type_);

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


	const char* XHWShader::getProfileFromType(ShaderType::Enum type)
	{
		switch (type)
		{
		case ShaderType::Vertex:
			return "vs_5_0";
		case ShaderType::Pixel:
			return "ps_5_0";
		case ShaderType::Geometry:
			return "gs_5_0";
		case ShaderType::Hull:
			return "hs_5_0";
		case ShaderType::Domain:
			return "ds_5_0";

		case ShaderType::UnKnown:
			break;
		}

		X_ASSERT_UNREACHABLE();
		return "";
	}

	std::pair<uint8_t, uint8_t> XHWShader::getProfileVersionForType(ShaderType::Enum type)
	{
		uint8_t major, minor;

		major = 0;
		minor = 0;

		switch (type)
		{
		case ShaderType::Vertex:
		case ShaderType::Pixel:
		case ShaderType::Geometry:
		case ShaderType::Hull:
		case ShaderType::Domain:
			major = 5;
			minor = 0;
			break;

		default:
			X_ASSERT_UNREACHABLE();
			break;
		}

		return std::make_pair(major, minor);
	}


	// -----------------------------------------------


	void XShaderTechniqueHW::release(void)
	{
		core::SafeRelease(pVertexShader);
		core::SafeRelease(pPixelShader);
		core::SafeRelease(pGeoShader);
		core::SafeRelease(pHullShader);
		core::SafeRelease(pDomainShader);
	}

	bool XShaderTechniqueHW::canDraw(void) const
	{
		bool canDraw = true;

		if (pVertexShader) {
			canDraw &= pVertexShader->getStatus() == ShaderStatus::ReadyToRock;
		}
		if (pPixelShader) {
			canDraw &= pPixelShader->getStatus() == ShaderStatus::ReadyToRock;
		}
		if (pGeoShader) {
			canDraw &= pGeoShader->getStatus() == ShaderStatus::ReadyToRock;
		}
		if (pHullShader) {
			canDraw &= pHullShader->getStatus() == ShaderStatus::ReadyToRock;
		}
		if (pDomainShader) {
			canDraw &= pDomainShader->getStatus() == ShaderStatus::ReadyToRock;
		}

		return canDraw;
	}

	void XShaderTechniqueHW::tryCompile(void)
	{

	}


#if 0


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

				pConstBuffers_[x][i] = X_NEW_ARRAY_ALIGNED(ID3D11Buffer*, size, g_rendererArena, "ShaderConstBuffers", X_ALIGN_OF(ID3D11Buffer*));
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


	// ------------------------------------------------------------------


	XShaderParam* XHWShader_Dx10::getParameter(const core::StrHash& nameHash)
	{
		core::Array<XShaderParam>::size_type i;
		for (i = 0; i < bindVars_.size(); i++)
		{
			if (bindVars_[i].nameHash == nameHash)
				return &bindVars_[i];
		}

		return nullptr;
	}

	// ====================== Static =============================


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

#if 1
		X_ASSERT_NOT_IMPLEMENTED();

#else
		uint32_t i, num;
		XShaderParam* pCurParam;
		Vec4f* pDest;

		num = numPrams;
		pDest = vecTemp;

		// render::DX11XRender* const __restrict r = &render::g_Dx11D3D;
		//	XHWShader_Dx10::InitBufferPointers();
		X_ASSERT_NOT_IMPLEMENTED();

		for (i = 0; i < num; i++)
		{

			if (pCurParam->flags.IsSet(ParamFlags::INT))
				setParameteri(pCurParam, pDest, shaderType, maxVecs);
			else
				setParameterf(pCurParam, pDest, shaderType, maxVecs);
		}
#endif
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




#endif

} // namespace shader

X_NAMESPACE_END