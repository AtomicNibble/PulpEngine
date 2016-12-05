#pragma once

#include <IShader.h>

#include <Hashing\xxHash.h>
#include <Containers\HashMap.h>
#include <unordered_set>

X_NAMESPACE_DECLARE(core,
	class XLexer;
);

X_NAMESPACE_BEGIN(render)

namespace shader
{
	// these are just types for loading the .shader format and holding hlsl source.

	X_DECLARE_FLAGS(TechniquePrams) (NAME, VERTEX_FNC, PIXEL_FNC);


	// a hlsl
	class SourceFile : public IShaderSource
	{
	public:
		typedef core::Array<SourceFile*> IncludedSourceArr;
		typedef std::unordered_set<core::string, core::hash<core::string>> RefrenceMap;

	public:
		SourceFile(core::MemoryArenaBase* arena);
		~SourceFile() = default;

		void writeSourceToFile(core::XFile* pFile) const;

		X_INLINE const core::string& getName(void) const;
		X_INLINE const core::string& getFileName(void) const;
		X_INLINE const core::string& getFileData(void) const;
		X_INLINE uint32_t getSourceCrc32(void) const;
		X_INLINE const IncludedSourceArr& getIncludeArr(void) const;
		X_INLINE IncludedSourceArr& getIncludeArr(void);
		X_INLINE ILFlags getILFlags(void) const X_FINAL;

		X_INLINE void setName(const core::string& name);
		X_INLINE void setFileName(const core::string& name);
		X_INLINE void setFileData(const core::string& name);
		X_INLINE void setSourceCrc32(uint32_t crc);
		X_INLINE void setILFlags(ILFlags flags);

		X_INLINE void addRefrence(const core::string& name);

	private:
		core::string name_;
		core::string fileName_;
		core::string fileData_;
		IncludedSourceArr includedFiles_;
		RefrenceMap refrences_;
		ILFlags ILFlags_;
		uint32_t sourceCrc32_;
	};


} // namespace shader

X_NAMESPACE_END


#include "ShaderSourceTypes.inl"