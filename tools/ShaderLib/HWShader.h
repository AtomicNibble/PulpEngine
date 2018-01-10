#pragma once


#include "ILTree.h"
#include <CBuffer.h>
#include <Sampler.h>
#include <Texture.h>
#include <Buffer.h>

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
		Ready,
		FailedToCompile
	);


	class ShaderBin;
	class SourceFile;

	X_ALIGNED_SYMBOL(class XHWShader, 64) : public IHWShader
	{
		typedef IShaderPermatation::LineraArray<XCBuffer> CBufferArr;
		typedef IShaderPermatation::LineraArray<Sampler> SamplerArr;
		typedef IShaderPermatation::LineraArray<Texture> TextureArr;
		typedef IShaderPermatation::LineraArray<Buffer> BufferArr;
		typedef core::Array<uint8_t> ByteArr;

		friend class ShaderBin;

	public:
		typedef core::Spinlock LockType;

	public:
		SHADERLIB_EXPORT XHWShader(core::MemoryArenaBase* arena, ShaderType::Enum type, const char* pName, 
			const core::string& entry, const core::string& customDefines,
			SourceFile* pSourceFile, PermatationFlags permFlags, ILFlags ILFlags);
		SHADERLIB_EXPORT ~XHWShader();

		X_INLINE const int32_t getID(void) const;
		X_INLINE void setID(int32_t id);

		X_INLINE LockType& getLock(void);

		X_INLINE const core::string& getName(void) const;
		X_INLINE const core::string& getEntryPoint(void) const;
		X_INLINE SourceFile* getShaderSource(void) const;
		X_INLINE PermatationFlags getPermFlags(void) const;
		X_INLINE ILFlags getILFlags(void) const;
		X_INLINE ShaderType::Enum getType(void) const;
		X_INLINE InputLayoutFormat::Enum getILFormat(void) const;
		X_INLINE int32_t getNumRenderTargets(void) const;
		X_INLINE int32_t getNumSamplers(void) const;
		X_INLINE int32_t getNumTextures(void) const;
		X_INLINE int32_t getNumConstantBuffers(void) const;
		X_INLINE int32_t getNumBuffers(void) const;
		X_INLINE int32_t getNumInputParams(void) const;
		X_INLINE int32_t getNumInstructions(void) const;
		X_INLINE CompileFlags getCompileFlags(void) const;

		X_INLINE ShaderStatus::Enum getStatus(void) const;
		X_INLINE bool isValid(void) const;
		X_INLINE bool isILFmtValid(void) const;
		X_INLINE bool hasFailedtoCompile(void) const;
		X_INLINE bool isCompiling(void) const;
		X_INLINE void markStale(void);


		X_INLINE const CBufferArr& getCBuffers(void) const;
		X_INLINE CBufferArr& getCBuffers(void);
		X_INLINE const BufferArr& getBuffers(void) const;
		X_INLINE BufferArr& getBuffers(void);
		X_INLINE const SamplerArr& getSamplers(void) const;
		X_INLINE SamplerArr& getSamplers(void);
		X_INLINE const TextureArr& getTextures(void) const;
		X_INLINE TextureArr& getTextures(void);
		X_INLINE const ByteArr& getShaderByteCode(void) const;

	public:
		SHADERLIB_EXPORT bool compile(const ByteArr& source, CompileFlags compileFlags);

	private:
		bool compileFromSource(const ByteArr& source, CompileFlags compileFlags);
		bool reflectShader(ID3D10Blob* pshaderBlob);

	protected:
		LockType lock_;
		
		core::string name_;
		core::string entryPoint_;
		core::string customDefines_;

		int32_t id_;

		SourceFile* pSourceFile_;

		// status
		ShaderStatus::Enum status_;
		// color, textured, skinned, instanced
		PermatationFlags permFlags_;
		// Vert / Pixel / Hull / Geo
		ShaderType::Enum type_;
		// POS_UV_COL_NOR
		InputLayoutFormat::Enum IlFmt_;

		ILFlags ILFlags_;

		// save info from shader reflection.
		int32_t numInputParams_;
		int32_t numRenderTargets_;
		int32_t numInstructions_;
		CompileFlags compileFlags_;

		CBufferArr cbuffers_;
		SamplerArr samplers_;
		TextureArr textures_;
		BufferArr buffers_;

		ByteArr bytecode_;
	};


} // namespace shader

X_NAMESPACE_END

#include "HWShader.inl"