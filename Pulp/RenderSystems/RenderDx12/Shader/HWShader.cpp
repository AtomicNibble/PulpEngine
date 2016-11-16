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

		ParamFlags VarTypeToFlags(const D3D12_SHADER_TYPE_DESC& CDesc)
		{
			ParamFlags f;

			if (CDesc.Class == D3D_SVC_MATRIX_COLUMNS)
			{
				f.Set(ParamFlag::MATRIX);
			}
			else if (CDesc.Class == D3D_SVC_VECTOR)
			{
				if (CDesc.Columns == 4) {
					f.Set(ParamFlag::VEC4);
				}
				else if (CDesc.Columns == 3) {
					f.Set(ParamFlag::VEC3);
				}
				else if (CDesc.Columns == 2) {
					f.Set(ParamFlag::VEC2);
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

			switch (CDesc.Type)
			{
				case D3D_SVT_BOOL:
					f.Set(ParamFlag::BOOL);
					break;
				case D3D_SVT_INT:
					f.Set(ParamFlag::INT);
					break;
				case D3D_SVT_FLOAT:
					f.Set(ParamFlag::FLOAT);
					break;
			}

			return f;
		}


		// we need a list of chicken ready for dippin!
		struct XParamDB
		{
			XParamDB(const char* pName_, ParamType::Enum type_) :
				XParamDB(pName_, type_, ParamFlags())
			{
			}

			XParamDB(const char* pName_, ParamType::Enum type_, ParamFlags flags_)
			{
				core::StackString<192, char> nameUpper(pName_);
				nameUpper.toUpper();

				pName = pName_;
				upperNameHash = core::StrHash(nameUpper.c_str(), nameUpper.length());
				type = type_;
				flags = flags_;		
			}


			const char* pName;
			core::StrHash upperNameHash;
			ParamType::Enum type;
			ParamFlags flags;
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

			XParamDB("worldToScreenMatrix", ParamType::PF_worldToScreenMatrix, ParamFlag::MATRIX),
			XParamDB("worldToCameraMatrix", ParamType::PF_worldToCameraMatrix, ParamFlag::MATRIX),
			XParamDB("cameraToWorldMatrix", ParamType::PF_cameraToWorldMatrix, ParamFlag::MATRIX),
			XParamDB("objectToWorldMatrix", ParamType::PI_objectToWorldMatrix, ParamFlag::MATRIX),

			XParamDB("worldMatrix", ParamType::PI_worldMatrix, ParamFlag::MATRIX),
			XParamDB("viewMatrix", ParamType::PI_viewMatrix, ParamFlag::MATRIX),
			XParamDB("projectionMatrix", ParamType::PI_projectionMatrix, ParamFlag::MATRIX),
			XParamDB("worldViewProjectionMatrix", ParamType::PI_worldViewProjectionMatrix, ParamFlag::MATRIX),



			XParamDB("time", ParamType::PF_Time, ParamFlag::FLOAT),
			XParamDB("frameTime", ParamType::PF_FrameTime, ParamFlag::FLOAT),
			XParamDB("screensize", ParamType::PF_ScreenSize, ParamFlag::VEC4),
			XParamDB("cameraPos", ParamType::PF_CameraPos),

			XParamDB("cameraFront", ParamType::PB_CameraFront),
			XParamDB("cameraRight", ParamType::PB_CameraRight),
			XParamDB("cameraUp", ParamType::PB_CameraUp),

		};

		const XParamDB* findParamBySematic(const char* pName)
		{
			const size_t num = sizeof(g_SematicParams) / sizeof(XParamDB);

			core::StackString<192, char> nameUpper(pName);
			core::StrHash upperNameHash(nameUpper.c_str(), nameUpper.length());

			for (size_t i = 0; i < num; i++)
			{
				const auto& param = g_SematicParams[i];

				if (upperNameHash != param.upperNameHash) {
					continue;
				}

				if (core::strUtil::IsEqualCaseInsen(pName, param.pName)) {
					return &param;
				}
			}
			return nullptr;
		}

	} // namespace

	// -------------------------------------------------------------------

	XHWShader::XHWShader(core::MemoryArenaBase* arena, XShaderManager& shaderMan,
			ShaderType::Enum type, const char* pName, const core::string& entry,
			SourceFile* pSourceFile, TechFlags techFlags) :
		shaderMan_(shaderMan),
		name_(pName),
		entryPoint_(entry),
		status_(ShaderStatus::NotCompiled),
		type_(type),
		IlFmt_(InputLayoutFormat::Invalid),
		numInputParams_(0),
		numRenderTargets_(0),
		numSamplers_(0),
		numTextures_(0),
		cbuffers_(arena),
		bytecode_(arena),
		D3DCompileflags_(0)
	{
		X_ASSERT_NOT_NULL(pSourceFile);

		sourceFileName_ = pSourceFile->getName();
		sourceCrc32_ = pSourceFile->getSourceCrc32();

	}


	bool XHWShader::compile(bool forceSync)
	{
		if (isValid()) {
			return true;
		}

		if (FailedtoCompile()) {
			return false;
		}

		ShaderBin& shaderBin = shaderMan_.getBin();
		if (loadFromCache(shaderBin)) {
			return true;
		}

		if (!loadSourceAndCompile(forceSync)) {
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

	bool XHWShader::loadSourceAndCompile(bool forceSync)
	{
		core::string source;

		// we need to get the whole file :D
		if (!shaderMan_.sourceToString(this->sourceFileName_, source))
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

		if (!forceSync && vars.asyncCompile())
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
#else
		D3DCompileflags_ |= D3DCOMPILE_OPTIMIZATION_LEVEL2;
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
				pErrStr = filterd.find(sourcName);

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

		if (!reflectShader()) {
			X_ERROR("Shader", "Failed to reflect shader");
			return false;
		}

		status_ = ShaderStatus::ReadyToRock;
		saveToCache(shaderMan_.getBin());

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

		if (FAILED(D3DReflect(bytecode_.data(), bytecode_.size(), IID_ID3D12ShaderReflection, (void **)&pShaderReflection)))
		{
			X_ERROR("Shader", "failed to reflect shader: %s", name_.c_str());
			return false;
		}

		D3D12_SHADER_DESC shaderDesc;
		pShaderReflection->GetDesc(&shaderDesc);

		X_LOG_BULLET;
		X_LOG0("Shader", "Instructions: %i", shaderDesc.InstructionCount);
		X_LOG0("Shader", "ConstantBuffers: %i", shaderDesc.ConstantBuffers);
		X_LOG0("Shader", "BoundResources: %i", shaderDesc.BoundResources);
		X_LOG0("Shader", "InputParameters: %i", shaderDesc.InputParameters);
		X_LOG0("Shader", "OutputParameters: %i", shaderDesc.OutputParameters);


		cbuffers_.setGranularity(shaderDesc.ConstantBuffers);

		for (uint32 n = 0; n<shaderDesc.ConstantBuffers; n++)
		{
			pCB = pShaderReflection->GetConstantBufferByIndex(n);

			D3D12_SHADER_BUFFER_DESC BufferDesc;
			pCB->GetDesc(&BufferDesc);

			if (BufferDesc.Type == D3D_CT_RESOURCE_BIND_INFO) {
				continue;
			}

			ConstbufType::Enum cbufType;

			if (core::strUtil::IsEqual("$Globals", BufferDesc.Name)) {
				cbufType = ConstbufType::PER_BATCH;
			}
			else if (core::strUtil::FindCaseInsensitive("PerFrameConstants", BufferDesc.Name)) {
				cbufType = ConstbufType::PER_FRAME;
			}
			else if (core::strUtil::FindCaseInsensitive("ObjectConstants", BufferDesc.Name)) {
				cbufType = ConstbufType::PER_INSTANCE;
			}
			else {
				X_WARNING("Shader", "Unknown cbuffer name, ignoring.");
				continue;
			}

			XCBuffer& cbuf = cbuffers_.AddOne(cbuffers_.getArena());
			cbuf.name = BufferDesc.Name;
			cbuf.size = safe_static_cast<int16_t>(BufferDesc.Size);
			cbuf.type = cbufType;
			cbuf.params.setGranularity(BufferDesc.Variables);

			for (uint32 i = 0; i<BufferDesc.Variables; i++)
			{
				ID3D12ShaderReflectionVariable* pCV = pCB->GetVariableByIndex(i);
				D3D12_SHADER_VARIABLE_DESC CDesc;
				pCV->GetDesc(&CDesc);

				// used and abused?
				if (!core::bitUtil::IsBitFlagSet(CDesc.uFlags, D3D_SVF_USED)) {
					continue;
				}

				D3D12_SHADER_TYPE_DESC CTDesc;
				ID3D12ShaderReflectionType* pVT = pCV->GetType();
				pVT->GetDesc(&CTDesc);

				const int32_t reg = (CDesc.StartOffset >> 4);

				XShaderParam& bind = cbuf.params.AddOne();
				bind.name = CDesc.Name;
				bind.nameHash = core::StrHash(CDesc.Name);
				bind.bind = safe_static_cast<int16_t>(reg);
				bind.slot = cbuf.type;
				bind.numParameters = (CDesc.Size + 15) >> 4; // vec4

				// a predefined param?
				const XParamDB* pEntry = findParamBySematic(CDesc.Name);
				if (!pEntry)
				{
					X_WARNING("Shader", "unknown input var must be set manualy: \"%s\"", CDesc.Name);	
					bind.flags = VarTypeToFlags(CTDesc);
					bind.type = ParamType::Unknown;
				}
				else
				{
					bind.flags = pEntry->flags;
					bind.type = pEntry->type;

					if (pEntry->flags.IsSet(ParamFlag::MATRIX))
					{
						if (CTDesc.Class != D3D_SVC_MATRIX_COLUMNS && CTDesc.Class != D3D_SVC_MATRIX_ROWS)
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
					else if (pEntry->flags.IsSet(ParamFlag::VEC4))
					{
						if (CTDesc.Class != D3D_SVC_VECTOR || CTDesc.Columns != 4)
						{
							X_ERROR("Shader", "input var: \"%s\" should be a float4", CDesc.Name);
							return false;
						}
					}
					else if (pEntry->flags.IsSet(ParamFlag::FLOAT))
					{
						if (CTDesc.Class != D3D_SVC_SCALAR || CTDesc.Columns != 1 || CTDesc.Rows != 1)
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
				}	
			}
		}

		D3D12_SHADER_INPUT_BIND_DESC InputBindDesc;
		for (uint32 n = 0; n<shaderDesc.BoundResources; n++)
		{
			core::zero_object(InputBindDesc);
			pShaderReflection->GetResourceBindingDesc(n, &InputBindDesc);

			if (InputBindDesc.Type == D3D_SIT_CBUFFER) 
			{
				// find the cbuffer?
				for (auto& cb : cbuffers_)
				{
					if (cb.name == InputBindDesc.Name)
					{
						cb.bindPoint = safe_static_cast<int16_t>(InputBindDesc.BindPoint);
						cb.bindCount = safe_static_cast<int16_t>(InputBindDesc.BindCount);
						break;
					}
				}		
			}
			else if (InputBindDesc.Type == D3D_SIT_SAMPLER)
			{
				numSamplers_++;
			}
			else if (InputBindDesc.Type == D3D_SIT_TEXTURE)
			{
				numTextures_++;

				// we still want to find a sampler for a given textures?
				// I will add that back in if a need it, but it would be nice to just support
				// samplers that are shared.
				// but thinking about it the sampling is defined per material.
				// so in order to control the sampling for each texture based on materials we need to have sampler per texture.
				// unless we generate a shader variant that has the correct samplers inserted...
				core::StackString512 textureResName(InputBindDesc.Name);

				for (uint32 i = 0; i < shaderDesc.BoundResources; i++)
				{
					pShaderReflection->GetResourceBindingDesc(i, &InputBindDesc);
					if (InputBindDesc.Type != D3D_SIT_SAMPLER) {
						continue;
					}

					core::StackString512 samplerResName(InputBindDesc.Name);

					// if the sampler state has the same name
					// as the texture link them.
					if (textureResName == samplerResName)
					{
						// we found it's sampler.
						break;
					}

					const size_t samplerSuffixLen = sizeof("sampler") - 1;
					if ((textureResName.length() + samplerSuffixLen) == samplerResName.length())
					{
						const char* pName = samplerResName.findCaseInsen(textureResName.begin());
						if (pName)
						{
							const size_t baseNameLen = textureResName.length();
							if (core::strUtil::IsEqualCaseInsen(pName + baseNameLen, samplerResName.end(), "sampler"))
							{
								// we found it's sampler.
								break;
							}
						}
					}
				}
			}

		}

		if (type_ == ShaderType::Vertex)
		{
			const ILTreeNode* pILnode = &shaderMan_.getILTree();
			D3D12_SIGNATURE_PARAMETER_DESC InputDsc;
			for (uint32 n = 0; n < shaderDesc.InputParameters; n++)
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
#if 0	// i just save number of output params don't store any extra info currently.
			D3D12_SIGNATURE_PARAMETER_DESC OutputDsc;
			for (uint32 n = 0; n < shaderDesc.OutputParameters; n++)
			{
				pShaderReflection->GetOutputParameterDesc(n, &OutputDsc);

				int goat = 0;
			}
#endif
		}

		pShaderReflection->Release();

		// save some data.
		if (type_ == ShaderType::Pixel) {
			numRenderTargets_ = safe_static_cast<int32_t>(shaderDesc.OutputParameters);
		}
		else {
			numRenderTargets_ = 0;
		}

		numInputParams_ = safe_static_cast<int32_t>(shaderDesc.InputParameters);
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

	CBufferLink::CBufferLink(ShaderType::Enum stage, const XCBuffer* pCBufer_) :
		stages(stage),
		pCBufer(pCBufer_)
	{

	}

	// -----------------------------------------------


	XShaderTechniqueHW::XShaderTechniqueHW(core::MemoryArenaBase* arena) :
		cbLinks(arena)
	{

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



	bool XShaderTechniqueHW::tryCompile(bool forceSync)
	{

		if (pVertexShader && pVertexShader->getStatus() == ShaderStatus::NotCompiled) {
			if (!pVertexShader->compile(forceSync)) {
				return false;
			}

			IlFmt = pVertexShader->getILFormat();
		}
		if (pPixelShader && pPixelShader->getStatus() == ShaderStatus::NotCompiled) {
			if (!pPixelShader->compile(forceSync)) {
				return false;
			}
		}

		// create a merged list of const buffers.
		// so that we know if any const buffers are shared between stages
		// so that we can set visibility flags in the root sig.
		addCbufs(pVertexShader);
		addCbufs(pPixelShader);

		return true;
	}

	void XShaderTechniqueHW::addCbufs(XHWShader* pShader)
	{
		const auto& cbufs = pShader->getCBuffers();
		for (const auto& cb : cbufs)
		{
			// we match by name and size currently.
			for (auto& link : cbLinks)
			{
				if (link.pCBufer->isEqual(cb))
				{
					link.stages.Set(pShader->getType());
					break;
				}
			}

			cbLinks.emplace_back(pShader->getType(), &cb);
		}
	}

} // namespace shader

X_NAMESPACE_END