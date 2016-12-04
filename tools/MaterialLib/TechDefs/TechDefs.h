#pragma once

#include <IMaterial.h>

#include <Containers\Array.h>
#include <Containers\HashMap.h>

#include <Traits\MemberFunctionTraits.h>
#include <Util\Delegate.h>

X_NAMESPACE_DECLARE(core,
	class XLexer;
)

X_NAMESPACE_BEGIN(engine)


	// This is like a file format for defining states and flags that the materials can select
	// Just makes it easy to define what state a material will have.
	//
	//	this format supports includes
	//	defining states: blend, stencil etc.
	//
	//
	class TechSetDef;

	class TechSetDefs
	{
		typedef core::Array<char> FileBuf;
		typedef core::HashMap<core::Path<char>, FileBuf> SourceMap;
		typedef core::HashMap<core::Path<char>, TechSetDef*> TechSetDefMap;

		static const char* INCLUDE_DIR;
		static const wchar_t* INCLUDE_DIR_W;
	public:

		typedef core::Array<core::string> CatTypeArr;

	public:
		MATLIB_EXPORT TechSetDefs(core::MemoryArenaBase* arena);
		MATLIB_EXPORT ~TechSetDefs();


		MATLIB_EXPORT static bool getTechCatTypes(MaterialCat::Enum cat, CatTypeArr& typesOut);
		MATLIB_EXPORT bool getTechDef(MaterialCat::Enum cat, const char* pName, TechSetDef*& pTechDefOut);
		MATLIB_EXPORT bool getTechDef(MaterialCat::Enum cat, const core::string& name, TechSetDef*& pTechDefOut);

		MATLIB_EXPORT void clearIncSrcCache(void);

	private:
		bool loadTechDef(const core::Path<char>& path, const core::string& name);
		static bool loadTechCat(MaterialCat::Enum cat, CatTypeArr& typesOut);

		bool loadFile(const core::Path<char>& path, FileBuf& bufOut);

		bool includeCallback(core::XLexer& lex, core::string& name, bool useIncludePath);

	private:
		core::MemoryArenaBase* arena_;

		TechSetDefMap techDefs_;
		SourceMap incSourceMap_;
	};


X_NAMESPACE_END
