#pragma once


#include "ILTree.h"
#include "CBuffer.h"

X_NAMESPACE_DECLARE(core,
	namespace V2 {
		struct Job;
		class JobSystem;
	}
)


X_NAMESPACE_BEGIN(render)

namespace shader
{

	X_DECLARE_FLAGS8(ShaderStatus) (NotCompiled, Compiling, AsyncCompileDone, ReadyToRock, FailedToCompile);

	class ShaderBin;
	class XShaderManager;
	class SourceFile;


	X_ALIGNED_SYMBOL(class XHWShader, 64)
	{
		typedef core::Array<XCBuffer> CBufferArr;
		typedef core::Array<uint8_t> ByteArr;

		friend class ShaderBin;

	public:
		XHWShader(core::MemoryArenaBase* arena, XShaderManager& shaderMan,
			ShaderType::Enum type, const char* pName, const core::string& entry,
			SourceFile* pSourceFile, TechFlags techFlags);

		X_INLINE const int32_t getID(void) const;
		X_INLINE void setID(int32_t id);

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
		X_INLINE int32_t getSourceCrc32(void) const;
		X_INLINE int32_t getD3DCompileFlags(void) const;

		X_INLINE ShaderStatus::Enum getStatus(void) const;
		X_INLINE bool isValid(void) const;
		X_INLINE bool isILFmtValid(void) const;
		X_INLINE bool FailedtoCompile(void) const;
		X_INLINE bool isCompiling(void) const;

		X_INLINE const CBufferArr& getCBuffers(void) const;
		X_INLINE CBufferArr& getCBuffers(void);
		X_INLINE const ByteArr& getShaderByteCode(void) const;

	public:
		bool compile(bool forceSync = false);
		bool invalidateIfChanged(uint32_t newSourceCrc32);

	private:
		void getShaderCompilePaths(core::Path<char>& src, core::Path<char>& dest);
		void getShaderCompileSrc(core::Path<char>& src);
		void getShaderCompileDest(core::Path<char>& dest);

		bool loadFromCache(ShaderBin& shaderBin);
		bool saveToCache(ShaderBin& shaderBin);

		bool loadSourceAndCompile(bool forceSync = false);
		bool compileFromSource(core::string& source);
		void CompileShader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
		bool reflectShader(void);

	protected:
		XShaderManager& shaderMan_;

		core::string name_;
		core::string sourceFileName_;
		core::string entryPoint_;
		core::string source_;

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
		int32_t numSamplers_;
		int32_t numTextures_;
		// the flags it was compiled with: DEBUG | OPT_LEVEL1 etc.
		uint32_t D3DCompileflags_;

		CBufferArr cbuffers_;

		ByteArr bytecode_;
	};


} // namespace shader

X_NAMESPACE_END

#include "HWShader.inl"