#include "stdafx.h"

#include "HWShader.h"
#include "ShaderUtil.h"
#include "ShaderBin.h"

#include <IConsole.h>
#include <IFileSys.h>
#include <Threading\JobSystem2.h>

#include <D3Dcompiler.h>
#include <../../3rdparty/source/directx/D3DX9Shader.h>

#include <Time\StopWatch.h>
#include <String\StringTokenizer.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{


	  // -------------------------------------------------------------------

	XHWShader::XHWShader(core::MemoryArenaBase* arena, ShaderType::Enum type, const char* pName, 
		const core::string& entry, const core::string& customDefines,
		SourceFile* pSourceFile, PermatationFlags permFlags) :
		name_(pName),
		pSourceFile_(X_ASSERT_NOT_NULL(pSourceFile)),
		entryPoint_(entry),
		customDefines_(customDefines),
		status_(ShaderStatus::NotCompiled),
		permFlags_(permFlags),
		type_(type),
		IlFmt_(InputLayoutFormat::Invalid),
		numInputParams_(0),
		numRenderTargets_(0),
		numInstructions_(0),
		cbuffers_(arena),
		samplers_(arena),
		textures_(arena),
		bytecode_(arena),
		id_(-1)
	{

		cbuffers_.setGranularity(2);
	}

	XHWShader::~XHWShader()
	{

	}

	bool XHWShader::compile(const ByteArr& source, CompileFlags compileFlags)
	{
		status_ = ShaderStatus::Compiling;
		bool res = compileFromSource(source, compileFlags);

		if (res) {
			status_ = ShaderStatus::ReadyToRock;
		}
		else
		{
			status_ = ShaderStatus::FailedToCompile;
		}

		return res;
	}


	bool XHWShader::compileFromSource(const ByteArr& source, CompileFlags flags)
	{
		X_LOG0("Shader", "Compiling shader: \"%s\" tid: %" PRIX32, name_.c_str(), core::Thread::GetCurrentID());

		compileFlags_ = flags;

		DWORD D3DCompileflags = 0;

		if (flags.IsSet(CompileFlag::Debug)) {
			D3DCompileflags |= D3DCOMPILE_DEBUG;
		}
		if (flags.IsSet(CompileFlag::TreatWarningsAsErrors)) {
			D3DCompileflags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
		}

		if (flags.IsSet(CompileFlag::OptimizationLvl3)) {
			D3DCompileflags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
		}
		else if (flags.IsSet(CompileFlag::OptimizationLvl2)) {
			D3DCompileflags |= D3DCOMPILE_OPTIMIZATION_LEVEL2;
		}
		else if (flags.IsSet(CompileFlag::OptimizationLvl1)) {
			D3DCompileflags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
		}
		else if (flags.IsSet(CompileFlag::OptimizationLvl0)) {
			D3DCompileflags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
		}

		if (flags.IsSet(CompileFlag::PackMatrixRowMajor)) {
			D3DCompileflags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
		}
		else if (flags.IsSet(CompileFlag::PackMatrixColumnMajor)) {
			D3DCompileflags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
		}

		// allow 16 flags.
		core::FixedArray<D3D_SHADER_MACRO, 17> macros;
		std::array<char, 2048> macroBuffer;
		size_t macroBufIdx = 0;

		// i turn all set flags into strings.
		if (permFlags_.IsAnySet())
		{
			core::StackString<256, char> macro;

			for (uint32_t i = 1; i < PermatationFlags::FLAGS_COUNT; i++)
			{
				Permatation::Enum flag = static_cast<PermatationFlags::Enum>(1 << i);
				if (permFlags_.IsSet(flag))
				{
					if (flag == Permatation::Color || flag == Permatation::Uv2)
					{
						macro.setFmt("IL_%s", Permatation::ToString(flag));
					}
					else
					{
						macro.setFmt("X_%s", Permatation::ToString(flag));
					}
					macro.toUpper();

					if (macroBufIdx + macro.length() > macroBuffer.size())
					{
						X_ERROR("Shader", "Failed to fit all macros in buffer");
						return false;
					}

					char* pStart = &macroBuffer[macroBufIdx];
					std::copy(macro.begin(), macro.end(), pStart);

					macroBufIdx += macro.length();
					macroBuffer[macroBufIdx++] = '\0';

					macros.push_back({ pStart, "1" });
				}
			}
		}

		if (customDefines_.isNotEmpty())
		{
			// need to split them into defins.
			core::StringRange<char> token(nullptr, nullptr);
			core::StringTokenizer<char> tokens(customDefines_.begin(), customDefines_.end(), ',');
			while (tokens.ExtractToken(token))
			{
				const size_t tokenLen =  token.GetLength();

				if (macroBufIdx + tokenLen > macroBuffer.size())
				{
					X_ERROR("Shader", "Failed to fit all macros in buffer");
					return false;
				}

				char* pStart = &macroBuffer[macroBufIdx];
				std::copy(token.GetStart(), token.GetEnd(), pStart);

				macroBufIdx += tokenLen;
				macroBuffer[macroBufIdx++] = '\0';

				macros.push_back({ pStart, "1" });
			}
		}

		// log the macros
		for (const auto& macro : macros)
		{
			X_LOG0("Shader", "Macro: name: \"%s\" value: \"%s\"", macro.Name, macro.Definition);
		}

		// add blank one.
		macros.push_back({ nullptr, nullptr });

		ID3DBlob* pBlob = nullptr;
		ID3DBlob* pErrorBlob = nullptr;

		core::StopWatch timer;
		core::string sourcName = name_ + ".fxcb.hlsl";

		const char* pEntry = entryPoint_.c_str();
		if (entryPoint_.isEmpty())
		{
			pEntry = DEFAULT_SHADER_ENTRY[type_];
		}

		HRESULT hr = D3DCompile(
			source.data(),
			source.size(),
			sourcName,
			macros.data(), // pDefines
			nullptr, // pInclude
			pEntry,
			Util::getProfileFromType(type_),
			D3DCompileflags, // Flags
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

				filterd.stripTrailing('\n');

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

		if (!reflectShader(pBlob)) {
			core::SafeReleaseDX(pBlob);
			X_ERROR("Shader", "Failed to reflect shader");
			return false;
		}

		{
			ID3DBlob* pStripped = nullptr;

			const size_t preStripSize = pBlob->GetBufferSize();

			hr = D3DStripShader(
				pBlob->GetBufferPointer(),
				pBlob->GetBufferSize(),
				D3DCOMPILER_STRIP_ROOT_SIGNATURE | // you silly slut.
				D3DCOMPILER_STRIP_REFLECTION_DATA |
				D3DCOMPILER_STRIP_TEST_BLOBS,
				&pStripped
			);

			if (SUCCEEDED(hr))
			{
				const size_t strippedBytes = preStripSize - pStripped->GetBufferSize();
				X_LOG2("Shader", "Stripped %" PRIuS " bytes from shader", strippedBytes);

				core::SafeReleaseDX(pBlob);
				pBlob = pStripped;
			}
			else
			{
				core::SafeReleaseDX(pStripped);
				X_ERROR("Shader", "Failed to strip blob: %x", hr);
			}
		}

		// copy the byte code.
		bytecode_.resize(pBlob->GetBufferSize());
		std::memcpy(bytecode_.data(), pBlob->GetBufferPointer(), pBlob->GetBufferSize());

		// release 
		core::SafeReleaseDX(pErrorBlob);
		core::SafeReleaseDX(pBlob);

		const float elapsed = timer.GetMilliSeconds();
		X_LOG0("Shader", "Compile complete: ^6%.3fms", elapsed);

		return true;
	}


	bool XHWShader::reflectShader(ID3D10Blob* pshaderBlob)
	{
		ID3D12ShaderReflection* pShaderReflection;
		ID3D12ShaderReflectionConstantBuffer* pCB;

		auto hr = D3DReflect(pshaderBlob->GetBufferPointer(), pshaderBlob->GetBufferSize(), IID_ID3D12ShaderReflection, (void **)&pShaderReflection);
		if(FAILED(hr))
		{
			X_ERROR("Shader", "D3D reflect failed(%" PRIi32 "): %s", hr, name_.c_str());
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

			UpdateFreq::Enum baseUpdateRate = UpdateFreq::FRAME;

			if (core::strUtil::IsEqual("$Globals", BufferDesc.Name)) {
				baseUpdateRate = UpdateFreq::BATCH;
			}
			else if (core::strUtil::FindCaseInsensitive("PerFrameConstants", BufferDesc.Name)) {
				baseUpdateRate = UpdateFreq::FRAME;
			}
			else if (core::strUtil::FindCaseInsensitive("ObjectConstants", BufferDesc.Name)) {
				baseUpdateRate = UpdateFreq::INSTANCE;
			}
			else {
				X_WARNING("Shader", "Unknown cbuffer name \"%s\", defaulting to per-frame freq.", BufferDesc.Name);
				baseUpdateRate = UpdateFreq::FRAME;
			}

			XCBuffer& cbuf = cbuffers_.AddOne(cbuffers_.getArena());
			cbuf.setName(BufferDesc.Name);
			cbuf.setSize(safe_static_cast<int16_t>(BufferDesc.Size));
			cbuf.setParamGranularitys(BufferDesc.Variables);
			cbuf.setUpdateRate(baseUpdateRate);

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

				XShaderParam bind;
				bind.setName(CDesc.Name);
				bind.setBindPoint(reg);
				bind.setSize(CDesc.Size);
				// bind.numParameters = (CDesc.Size + 15) >> 4; // vec4

				// a predefined param?
				const Util::XParamDB* pEntry = Util::getPredefinedParamForName(CDesc.Name);
				if (!pEntry)
				{
					X_WARNING("Shader", "unknown input var must be set manualy: \"%s\"", CDesc.Name);
					bind.setFlags(Util::VarTypeToFlags(CTDesc));
					bind.setType(ParamType::Unknown);
					bind.setUpdateRate(UpdateFreq::UNKNOWN);
				}
				else
				{
					bind.setFlags(pEntry->flags);
					bind.setType(pEntry->type);
					bind.setUpdateRate(pEntry->updateRate);

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


				cbuf.addParam(std::move(bind));
			}

			cbuf.postPopulate();
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
					if (cb.getName() == InputBindDesc.Name)
					{
						cb.setBindPointAndCount(
							safe_static_cast<int16_t>(InputBindDesc.BindPoint),
							safe_static_cast<int16_t>(InputBindDesc.BindCount)
						);
						break;
					}
				}
			}
			else if (InputBindDesc.Type == D3D_SIT_SAMPLER)
			{
				samplers_.emplace_back(
					InputBindDesc.Name,
					safe_static_cast<int16_t>(InputBindDesc.BindPoint),
					safe_static_cast<int16_t>(InputBindDesc.BindCount)
				);
			}
			else if (InputBindDesc.Type == D3D_SIT_TEXTURE)
			{
				texture::TextureType::Enum type = texture::TextureType::UNKNOWN;

				switch (InputBindDesc.Dimension)
				{
					case D3D_SRV_DIMENSION_TEXTURE1D:
						type = texture::TextureType::T1D;
						break;
					case D3D_SRV_DIMENSION_TEXTURE2D:
						type = texture::TextureType::T2D;
						break;
					case D3D_SRV_DIMENSION_TEXTURE3D:
						type = texture::TextureType::T3D;
						break;
					case D3D_SRV_DIMENSION_TEXTURECUBE:
						type = texture::TextureType::TCube;
						break;
					default:
						X_WARNING("Shader", "Unhandled texture dimension: %" PRIi32, InputBindDesc.Dimension);
						break;
				}

				textures_.emplace_back(
					InputBindDesc.Name,
					safe_static_cast<int16_t>(InputBindDesc.BindPoint),
					safe_static_cast<int16_t>(InputBindDesc.BindCount),
					type
				);
			}

		}

		if (type_ == ShaderType::Vertex)
		{
			const ILTreeNode* pILnode = ILTreeNode::getILTree();
			D3D12_SIGNATURE_PARAMETER_DESC InputDsc;
			for (uint32 n = 0; n < shaderDesc.InputParameters; n++)
			{
				pShaderReflection->GetInputParameterDesc(n, &InputDsc);

				// how many?
				// i only do sematic checks now, since i don't give a flying fuck about the format.

				if (permFlags_.IsSet(Permatation::Instanced) && InputDsc.SemanticIndex > 0)
				{
					// pos and color with semantic index above 0 are ignores when instancing enabled.
					if (core::strUtil::IsEqual("POSITION", InputDsc.SemanticName))
					{
						continue;
					}
					if (core::strUtil::IsEqual("COLOR", InputDsc.SemanticName))
					{
						continue;
					}
				}

				pILnode = pILnode->GetChildWithSemtaic(InputDsc.SemanticName);
				if (!pILnode)
				{
					X_ERROR("Shader", "Input layout invalid.");
					return false;
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

		X_WARNING_IF(type_ != ShaderType::Pixel && samplers_.isNotEmpty(), "Shader", "None pixel shader has samplers, currently this is not supported");

		pShaderReflection->Release();

		// save some data.
		if (type_ == ShaderType::Pixel) {
			numRenderTargets_ = safe_static_cast<int32_t>(shaderDesc.OutputParameters);
		}
		else {
			numRenderTargets_ = 0;
		}

		numInstructions_ = safe_static_cast<int32_t>(shaderDesc.InstructionCount);
		numInputParams_ = safe_static_cast<int32_t>(shaderDesc.InputParameters);
		return true;
	}

	// -----------------------------------------------



} // namespace shader

X_NAMESPACE_END