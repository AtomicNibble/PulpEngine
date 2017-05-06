#pragma once


#include "ILTree.h"
#include <CBuffer.h>
#include <Sampler.h>
#include <Texture.h>

X_NAMESPACE_DECLARE(core,
	namespace V2 {
	struct Job;
	class JobSystem;
}
)

struct ID3D10Blob;

X_NAMESPACE_BEGIN(render)

namespace shader
{
	X_DECLARE_FLAGS8(ShaderStatus) (
		NotCompiled,
		Compiling,
		AsyncCompileDone,
		ReadyToRock,
		FailedToCompile
	);


	class ShaderBin;

	X_ALIGNED_SYMBOL(class XHWShader, 64) : public IHWShader
	{
		typedef core::Array<XCBuffer> CBufferArr;
		typedef core::Array<Sampler> SamplerArr;
		typedef core::Array<Texture> TextureArr;
		typedef core::Array<uint8_t> ByteArr;

		friend class ShaderBin;

	public:
		typedef core::Spinlock LockType;

	public:
		SHADERLIB_EXPORT XHWShader(core::MemoryArenaBase* arena, ShaderType::Enum type, const char* pName, const core::string& entry,
			const core::string& sourceFile, uint32_t soruceFilecrc32, TechFlags techFlags);
		SHADERLIB_EXPORT ~XHWShader();

		X_INLINE const int32_t getID(void) const;
		X_INLINE void setID(int32_t id);

		X_INLINE LockType& getLock(void);

		X_INLINE const core::string& getName(void) const;
		X_INLINE const core::string& getSourceFileName(void) const;
		X_INLINE const core::string& getEntryPoint(void) const;
		X_INLINE TechFlags getTechFlags(void) const;
		X_INLINE ShaderType::Enum getType(void) const;
		X_INLINE InputLayoutFormat::Enum getILFormat(void) const;
		X_INLINE int32_t getNumRenderTargets(void) const;
		X_INLINE int32_t getNumSamplers(void) const;
		X_INLINE int32_t getNumTextures(void) const;
		X_INLINE int32_t getNumConstantBuffers(void) const;
		X_INLINE int32_t getNumInputParams(void) const;
		X_INLINE int32_t getNumInstructions(void) const;
		X_INLINE int32_t getSourceCrc32(void) const;
		X_INLINE CompileFlags getCompileFlags(void) const;

		X_INLINE ShaderStatus::Enum getStatus(void) const;
		X_INLINE bool isValid(void) const;
		X_INLINE bool isILFmtValid(void) const;
		X_INLINE bool FailedtoCompile(void) const;
		X_INLINE bool isCompiling(void) const;

		X_INLINE const CBufferArr& getCBuffers(void) const;
		X_INLINE CBufferArr& getCBuffers(void);
		X_INLINE const SamplerArr& getSamplers(void) const;
		X_INLINE SamplerArr& getSamplers(void);
		X_INLINE const TextureArr& getTextures(void) const;
		X_INLINE TextureArr& getTextures(void);
		X_INLINE const ByteArr& getShaderByteCode(void) const;

	public:
		SHADERLIB_EXPORT bool compile(core::string& source, CompileFlags compileFlags);

	private:
		bool compileFromSource(core::string& source, CompileFlags compileFlags);
		bool reflectShader(ID3D10Blob* pshaderBlob);

	protected:
		LockType lock_;
		
		core::string name_;
		core::string sourceFileName_;
		core::string entryPoint_;

		int32_t id_;
		uint32_t sourceCrc32_; // the crc of the source this was compiled from.

		// status
		ShaderStatus::Enum status_;
		// color, textured, skinned, instanced
		TechFlags techFlags_;
		// Vert / Pixel / Hull / Geo
		ShaderType::Enum type_;
		// POS_UV_COL_NOR
		InputLayoutFormat::Enum IlFmt_;

		// save info from shader reflection.
		int32_t numInputParams_;
		int32_t numRenderTargets_;
		int32_t numInstructions_;
		CompileFlags compileFlags_;

		CBufferArr cbuffers_;
		SamplerArr samplers_;
		TextureArr textures_;

		ByteArr bytecode_;
	};


} // namespace shader

X_NAMESPACE_END

#include "HWShader.inl"