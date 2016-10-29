#pragma once

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

	X_DECLARE_ENUM(PreProType)(Include, Define, Undef, If, IfDef, IfNDef, Else, EndIF);
	X_DECLARE_FLAGS(TechniquePrams) (NAME, VERTEX_FNC, PIXEL_FNC);


	struct PrePro
	{
		PreProType::Enum type;
		core::string expression;
	};


	// a hlsl
	class SourceFile
	{
	public:
		typedef core::Array<SourceFile*> IncludedSourceArr;
		typedef core::Array<PrePro> PreProArr;
		typedef std::unordered_set<core::string, core::hash<core::string>> RefrenceMap;

	public:
		SourceFile(core::MemoryArenaBase* arena);
		~SourceFile() = default;

		X_INLINE const core::string& getName(void) const;
		X_INLINE const core::string& getFileName(void) const;
		X_INLINE const core::string& getFileData(void) const;
		X_INLINE uint32_t getSourceCrc32(void) const;
		X_INLINE const IncludedSourceArr& getIncludeArr(void) const;
		X_INLINE IncludedSourceArr& getIncludeArr(void);
		X_INLINE ILFlags getILFlags(void);

		X_INLINE void setName(const core::string& name);
		X_INLINE void setFileName(const core::string& name);
		X_INLINE void setFileData(const core::string& name);
		X_INLINE void setSourceCrc32(uint32_t crc);

		X_INLINE void addRefrence(const core::string& name);

	private:
		core::string name_;
		core::string fileName_;
		core::string fileData_;
		IncludedSourceArr includedFiles_;
		PreProArr prePros_;
		RefrenceMap refrences_;
		ILFlags ILFlags_;
		uint32_t sourceCrc32_;
	};


	class ShaderSourceFileTechnique
	{
	public:
		ShaderSourceFileTechnique();
		~ShaderSourceFileTechnique() = default;

		bool parse(core::XLexer& lex);

		X_INLINE const core::string& getName(void) const;
		X_INLINE const core::string& getVertexFunc(void) const;
		X_INLINE const core::string& getPixelFunc(void) const;

		X_INLINE const TechFlags getTechFlags(void) const;

	private:
		bool processName(void);

	private:
		core::string name_;
		core::string vertex_func_;
		core::string pixel_func_;

		Flags<TechniquePrams> flags_;
		TechFlags techFlags_;
	};

	class ShaderSourceFile
	{
		typedef core::Array<ShaderSourceFileTechnique> TechArr;

	public:
		ShaderSourceFile(core::MemoryArenaBase* arena);
		~ShaderSourceFile() = default;

		X_INLINE size_t numTechs(void) const;

		void addTech(const ShaderSourceFileTechnique& tech);

	public:
		core::string name_;
		SourceFile* pFile_;
		SourceFile* pHlslFile_;
		uint32_t sourceCrc32_;
		uint32_t hlslSourceCrc32_;
		TechArr techniques_;
	};

} // namespace shader

X_NAMESPACE_END


#include "ShaderSourceTypes.inl"