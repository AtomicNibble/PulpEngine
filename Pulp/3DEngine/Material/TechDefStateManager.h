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
	typedef render::shader::ShaderSourceArr ShaderSourceArr;
	typedef core::Array<TechDefPerm> TechDefPermArr;

public:
	TechDef(core::MemoryArenaBase* arena);

	TechDefPerm* getPerm();

public:
	core::StrHash nameHash;
	core::string name;

	render::StateDesc stateDesc;

	// the source files every perm is made from
	ShaderSourceArr shaderSource;
	TechDefPermArr perms_;
};

class TechDefState
{
	typedef core::Array<TechDef> TechDefArr;

public:
	TechDefState(core::MemoryArenaBase* arena);

	TechDef* getTech(core::StrHash hash);

public:
	MaterialCat::Enum cat;
	core::string name;

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