#pragma once

#include <Containers\Array.h>
#include <Containers\HashIndex.h>

#include <String\StringHash.h>

X_NAMESPACE_BEGIN(engine)



struct TechDefPerm
{
	render::shader::IShaderPermatation* pShaderPerm;
	render::StateHandle stateHandle;
};


class TechDef
{
	typedef std::array<core::string, render::shader::ShaderStage::FLAGS_COUNT> ShaderEntryArr;

	typedef render::shader::ShaderSourceArr ShaderSourceArr;
	typedef core::Array<TechDefPerm> TechDefPermArr;
	typedef core::Array<BoundTexture> BoundTexturesArr;

public:
	TechDef(core::MemoryArenaBase* arena);

	TechDefPerm* getOrCreatePerm(render::shader::VertexFormat::Enum vertFmt);

	X_INLINE BoundTexturesArr::size_type getNumBoundTextures(void) const;
	X_INLINE const BoundTexturesArr& getBoundTextures(void) const;

public:
	core::StrHash nameHash;
	core::string name;

	render::StateDesc stateDesc;

	// the source files every perm is made from
	ShaderEntryArr shaderEntry_;
	ShaderSourceArr shaderSource_;
	TechDefPermArr perms_;
	BoundTexturesArr boundTextures_;
};

class TechDefState
{
	typedef core::Array<TechDef> TechDefArr;

public:
	TechDefState(core::MemoryArenaBase* arena);

	TechDef* getTech(core::StrHash hash);

public:
	MaterialCat::Enum cat;
	core::string name; // not a tech name.

	// the techs defined eg: "unlit", "depth", "flyinggoat"
	TechDefArr techs_;

	// the tech set def we got the techs and state from.
	TechSetDef* pTechSecDef_;
};

// this is to store states for each of the techSets.
class TechDefStateManager
{
	// store pointers since we return pointers so growing of this would invalidate returned pointers.
	typedef core::Array<TechDefState*> TechStatesArr;

public:
	TechDefStateManager(core::MemoryArenaBase* arena);
	~TechDefStateManager();


	TechDefState* getTechDefState(const MaterialCat::Enum cat, const core::string& name);

private:
	TechDefState* loadTechDefState(const MaterialCat::Enum cat, const core::string& name);


private:
	core::MemoryArenaBase* arena_;
	core::XHashIndex hashIndex_;
	TechStatesArr techStates_;

	TechSetDefs* pTechDefs_;
};


X_NAMESPACE_END

#include "TechDefStateManager.inl"