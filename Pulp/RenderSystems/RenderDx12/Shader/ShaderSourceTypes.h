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
		X_INLINE Flags<ILFlag> getILFlags(void);

		X_INLINE void setName(const core::string& name);
		X_INLINE void setFileName(const core::string& name);
		X_INLINE void setFileData(const core::string& name);
		X_INLINE void setSourceCrc32(uint32_t crc);

		X_INLINE void addRefrence(const core::string& name);

	protected:
		core::string name_;
		core::string fileName_;
		core::string fileData_;
		IncludedSourceArr includedFiles_;
		PreProArr prePros_;
		RefrenceMap refrences_;
		Flags<ILFlag> ILFlags_;
		uint32_t sourceCrc32_;
	};


	class ShaderSourceFile
	{
	public:

		class Technique
		{
		public:
			Technique();
			~Technique() = default;

			bool parse(core::XLexer& lex);
			bool processName(void);

		private:
			bool parseBlend(BlendInfo& blend, const char* name,
				const core::StackString512& key, const core::StackString512& value);


		private:
			core::string name_;
			core::string vertex_func_;
			core::string pixel_func_;

			BlendInfo src_;
			BlendInfo dst_;

			render::CullMode::Enum cullMode_;
			bool depth_write_;

			render::StateFlag state_;
			Flags<TechniquePrams> flags_;
			Flags<TechFlag> techFlags_;
		};

	public:
		ShaderSourceFile(core::MemoryArenaBase* arena);
		~ShaderSourceFile() = default;

		X_INLINE size_t numTechs(void) const;

		void addTech(const Technique& tech);

	public:
		core::string name_;
		SourceFile* pFile_;
		SourceFile* pHlslFile_;
		uint32_t sourceCrc32_;
		uint32_t hlslSourceCrc32_;
		core::Array<Technique> techniques_;
	};

} // namespace shader

X_NAMESPACE_END


#include "ShaderSourceTypes.inl"