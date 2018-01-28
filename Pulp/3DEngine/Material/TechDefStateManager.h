#pragma once

#include <Containers\Array.h>
#include <Containers\HashIndex.h>

#include <String\StringHash.h>

X_NAMESPACE_BEGIN(engine)


class TechDef
{
	typedef std::array<core::string, render::shader::ShaderStage::FLAGS_COUNT> ShaderStageStrArr;

	typedef render::shader::ShaderSourceArr ShaderSourceArr;
	typedef core::Array<TechDefPerm*> TechDefPermArr;

public:
	TechDef(const core::string& name, core::StrHash nameHash, const techset::TechSetDef* pTechSecDef, core::MemoryArenaBase* arena);
	~TechDef();

	TechDefPerm* getOrCreatePerm(render::shader::VertexFormat::Enum vertFmt, PermatationFlags permFlags);

	X_INLINE techset::Shader::AliaseArr::size_type getNumAliases(void) const;
	X_INLINE const techset::Shader::AliaseArr& getAliases(void) const;

public:
	core::MemoryArenaBase* permArena_;
	const techset::TechSetDef* pTechSecDef_; // the tech def this belongs to.
	core::StrHash nameHash_;
	core::string name_;

	render::StateDesc stateDesc_;

	core::CriticalSection permLock_;
	TechDefPermArr perms_;

	// the source files every perm is made from
	ShaderStageStrArr shaderDefines_;
	ShaderStageStrArr shaderEntry_;
	ShaderSourceArr shaderSource_;
	techset::Shader::AliaseArr aliases_;
};

class TechDefState
{
	typedef core::Array<TechDef> TechDefArr;

public:
	TechDefState(MaterialCat::Enum cat, const core::string& name, core::MemoryArenaBase* arena);
	~TechDefState();

	TechDef* getTech(core::StrHash hash);

public:
	MaterialCat::Enum cat_;
	core::string name_; // not a tech name.

	// the techs defined eg: "unlit", "depth", "flyinggoat"
	TechDefArr techs_;

	// the tech set def we got the techs and state from.
	techset::TechSetDef* pTechSecDef_;
};

// this is to store states for each of the techSets.
class TechDefStateManager
{
	typedef std::pair<MaterialCat::Enum, core::string> TechCatNamePair;

	struct tech_pair_hash
	{
		size_t operator()(const TechCatNamePair& p) const
		{
			// just add the cat enum, not a great hash but should be fine for this.
			return core::Hash::Fnv1aHash(p.second.data(), p.second.length()) + p.first;
		}
	};

	// store pointers since we return pointers so growing of this would invalidate returned pointers.
	typedef core::Array<TechDefState*> TechStatesArr;
	typedef core::HashMap<TechCatNamePair, TechDefState*, tech_pair_hash> TechStatesMap;

	typedef core::MemoryArena<
		core::PoolAllocator,
		core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
		core::SimpleBoundsChecking,
		core::SimpleMemoryTracking,
		core::SimpleMemoryTagging
#else
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
	> PoolArena;

public:
	TechDefStateManager(core::MemoryArenaBase* arena);
	~TechDefStateManager();

	void shutDown(void);


	TechDefState* getTechDefState(const MaterialCat::Enum cat, const core::string& name);

private:
	TechDefState* loadTechDefState(const MaterialCat::Enum cat, const core::string& name);


private:
	core::MemoryArenaBase* arena_;

	core::CriticalSection	cacheLock_;
	TechStatesMap			techs_;

	core::HeapArea			techsPoolHeap_;
	core::PoolAllocator		techsPoolAllocator_;
	PoolArena				techsPoolArena_;

private:
	techset::TechSetDefs* pTechDefs_;
};


X_NAMESPACE_END

#include "TechDefStateManager.inl"