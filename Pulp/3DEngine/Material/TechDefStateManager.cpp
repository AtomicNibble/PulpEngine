#include "stdafx.h"
#include "TechDefStateManager.h"

#include <Hashing\Fnva1Hash.h>
#include <Util\UniquePointer.h>

X_NAMESPACE_BEGIN(engine)

TechDef::TechDef(core::MemoryArenaBase* arena) :
	permArena_(arena),
	perms_(arena),
	aliases_(arena)
{
	perms_.setGranularity(8);
	aliases_.setGranularity(2);
	shaderSource_.fill(nullptr);
}

TechDef::~TechDef()
{
	render::IRender* pRenderSys = gEnv->pRender;
	X_ASSERT_NOT_NULL(pRenderSys);

	for (auto* pPerm : perms_)
	{
		pRenderSys->releaseShaderPermatation(pPerm->pShaderPerm);
		X_DELETE(pPerm, permArena_);
	}
}


TechDefPerm* TechDef::getOrCreatePerm(render::shader::VertexFormat::Enum vertFmt, PermatationFlags permFlags)
{
	// ok so this will be called to get a permatation.
	// we want to lazy compile these i think.
	// so how do we know what is supported.
	
	TechDefPerm* pCompilingPerm = nullptr;
	bool notCompiled = false;

	{
		core::CriticalSection::ScopedLock lock(permLock_);

		// ry find it.
		for (auto& pPerm : perms_)
		{
			if (pPerm->vertFmt == vertFmt && pPerm->permFlags == permFlags)
			{
				// we found one is it compiled?
				if (pPerm->status == TechStatus::COMPILED) {
					return pPerm;
				}

				if (pPerm->status == TechStatus::ERROR) {
					return nullptr;
				}

				// we must wait for it to become compiled.
				pCompilingPerm = pPerm;
				break;
			}
		}

		if (!pCompilingPerm)
		{
			notCompiled = true;

			// we don't have a perm, another thread might be compiling it tho.
			pCompilingPerm = X_NEW(TechDefPerm, permArena_, "TechDefPerm");
			pCompilingPerm->status = TechStatus::NOT_COMPILED;
			pCompilingPerm->permFlags = permFlags;
			pCompilingPerm->vertFmt = vertFmt;
			perms_.append(pCompilingPerm);
		}
		else
		{
			X_ASSERT(notCompiled == false, "Perm is already compiling")(notCompiled);
		}
	}

	// now we have left lock scope we wait for the perm to compile.
	// only if we are not the thread that is compiling it.
	if (!notCompiled)
	{
		while (pCompilingPerm->status == TechStatus::NOT_COMPILED)
		{
			core::Thread::Yield();
		}

		if (pCompilingPerm->status == TechStatus::ERROR) {
			return nullptr;
		}

		return pCompilingPerm;
	}

	// compile the perm.
	render::IRender* pRenderSys = gEnv->pRender;

	// for now just make one :|

	// create the hardware shaders
	render::shader::ShaderStagesArr stages{};

	for (size_t i=0; i<shaderSource_.size(); i++)
	{
		auto* pSource = shaderSource_[i];
		if (!pSource) {
			continue;
		}

		const auto type = static_cast<render::shader::ShaderType::Enum>(i);

		// create a instance of the shader with the flags we want it compiled with.
		// this won't actually compile it.
		stages[type] = pRenderSys->createHWShader(type, shaderEntry_[type], pSource, permFlags);
	}


	render::RenderTargetFmtsArr rtfs;
	rtfs.append(texture::Texturefmt::R8G8B8A8);

	// this will result in all shaders been compiled and cbuffer info created.
	render::shader::IShaderPermatation* pPerm = pRenderSys->createPermatation(stages);
	if (!pPerm) {
		X_ERROR("Tech", "Failed to create perm");
		pCompilingPerm->status = TechStatus::ERROR;
		return false;
	}

	auto passHandle = pRenderSys->createPassState(rtfs);
	if (passHandle == render::INVALID_STATE_HANLDE)
	{
		X_ERROR("Tech", "Failed to create passState");
		pRenderSys->releaseShaderPermatation(pPerm);
		pCompilingPerm->status = TechStatus::ERROR;
		return false;
	}

	// make a copy of the state and alter certain states.
	// anything that is actually defined in the techdef's should not be overriden here.
	// only stuff like vertex format which is runtime etc..
	decltype(stateDesc) stateDescCpy = stateDesc;
	stateDescCpy.vertexFmt = vertFmt;

	// come up with better way to send this data to render?
	// or does it just make sense to be part of state..?
	if (permFlags.IsSet(PermatationFlags::VertStreams)) {
		stateDescCpy.stateFlags.Set(render::StateFlag::VERTEX_STREAMS);
	}
	if (permFlags.IsSet(PermatationFlags::Instanced)) {
		stateDescCpy.stateFlags.Set(render::StateFlag::INSTANCED_POS_COLOR);
	}

	auto stateHandle = pRenderSys->createState(passHandle, pPerm, stateDescCpy, nullptr, 0);

	if (stateHandle == render::INVALID_STATE_HANLDE)
	{
		X_ERROR("Tech", "Failed to create state");
		pRenderSys->destoryPassState(passHandle);
		pRenderSys->releaseShaderPermatation(pPerm);
		pCompilingPerm->status = TechStatus::ERROR;
		return false;
	}

	pCompilingPerm->stateHandle = stateHandle;
	pCompilingPerm->pShaderPerm = pPerm;
	pCompilingPerm->status = TechStatus::COMPILED; // any other threads waiting on this can now return.
	return pCompilingPerm;
}


// -----------------------------------------------------------------------

TechDefState::TechDefState(core::MemoryArenaBase* arena) :
	techs_(arena)
{

}

TechDefState::~TechDefState()
{

}


TechDef* TechDefState::getTech(core::StrHash hash)
{
	for (auto& tech : techs_) {
		if (tech.nameHash == hash) {
			return &tech;
		}
	}

	return nullptr;
}

// -----------------------------------------------------------------------

TechDefStateManager::TechDefStateManager(core::MemoryArenaBase* arena) :
	arena_(arena),
	hashIndex_(arena, 256, 256),
	techStates_(arena)
{
	techStates_.reserve(64);
	techStates_.setGranularity(32);

	pTechDefs_ = X_NEW(techset::TechSetDefs, g_3dEngineArena, "MatManTechSets")(g_3dEngineArena);

}

TechDefStateManager::~TechDefStateManager()
{
	shutDown();
}

void TechDefStateManager::shutDown(void)
{
	for (auto* pTechDefState : techStates_)
	{
		X_DELETE(pTechDefState, arena_);
	}

	techStates_.free();

	if (pTechDefs_) {
		X_DELETE_AND_NULL(pTechDefs_, g_3dEngineArena);
	}
}


TechDefState* TechDefStateManager::getTechDefState(const MaterialCat::Enum cat, const core::string& name)
{

	// ok so we want to see if we already have the state for this tech def.
	// if not we must load the tech def and also create state.
	// can we do any optiomisation based on cat enum?
	// maybe if we have all techdefs in contigious block.
	// and then a seperate hash based index lookup.
	// that way techdefstates reside in memory together and lookup map is only used in loading so nicer if 
	// stored seperate so it will get purged out cache later.
	// do i want states of simular cat to be near each other in memory?
	// dunno.
	// 

	auto hashVal = core::Hash::Fnv1aHash(name.c_str(), name.length());
	
	{
		core::CriticalSection::ScopedLock lock(cacheLock_);

		for (auto idx = hashIndex_.first(hashVal); idx >= 0; idx = hashIndex_.next(idx))
		{
			auto* pTechDef = techStates_[idx];

			if (pTechDef->cat == cat && pTechDef->name == name)
			{
				return pTechDef;
			}
		}
	}

	// not in cache.
	auto* pTechDef = loadTechDefState(cat, name);
	if (!pTechDef) {
		X_ERROR("TechDefState", "Failed to load tech state def: %s:%s", MaterialCat::ToString(cat), name.c_str());
		return nullptr;
	}

	// add to cache
	core::CriticalSection::ScopedLock lock(cacheLock_);

	auto idx = techStates_.append(pTechDef);
	hashIndex_.insertIndex(hashVal, safe_static_cast<int32_t>(idx));

	return pTechDef;
}

TechDefState* TechDefStateManager::loadTechDefState(const MaterialCat::Enum cat, const core::string& name)
{
	core::CriticalSection::ScopedLock lock(cacheLock_);

	techset::TechSetDef* pTechDef = nullptr;
	if (!pTechDefs_->getTechDef(cat, name, pTechDef)) {
		X_ERROR("TechDefState", "Failed to get techdef definition for state creation");
		return nullptr;
	}

	// this don't cause any state to be created.
	// permatations are created on demand.
	core::UniquePointer<TechDefState> pTechDefState = core::makeUnique<TechDefState>(arena_, arena_);

	pTechDefState->cat = cat;
	pTechDefState->name = name;
	pTechDefState->techs_.setGranularity(1);
	pTechDefState->techs_.reserve(pTechDef->numTechs());
	pTechDefState->pTechSecDef_ = pTechDef;

	render::IRender* pRenderSys = gEnv->pRender;

	// we process all the techs 
	for (auto it = pTechDef->techBegin(); it != pTechDef->techEnd(); ++it)
	{
		const auto& techName = it->first;
		const auto& techDefTech = it->second;

		auto& tech = pTechDefState->techs_.AddOne(arena_);
		tech.nameHash = core::StrHash(techName.c_str(), techName.length());
		tech.name = techName;
		tech.stateDesc = techDefTech.state;

		for (uint32_t i = 0; i < render::shader::ShaderType::ENUM_COUNT - 1; i++)
		{
			const auto type = static_cast<render::shader::ShaderType::Enum>(i);
			const auto stage = staderTypeToStageFlag(type);

			if (!techDefTech.stages.IsSet(stage)) {
				continue;
			}

			const auto& shader = techDefTech.shaders[type];

			//  make sure the shader in the vertex slot is a vertex shader etc..
			X_ASSERT(shader.type == type, "Incorrect shader type for stage index.")(shader.type, type);

			// we ask for the source now, so we know we are able to atleast attempt to compile permatations later on.
			// and don't have to ask for them each time we make a perm.
			render::shader::IShaderSource* pShaderSource = pRenderSys->getShaderSource(shader.source.c_str());
			if (!pShaderSource) {
				return nullptr;
			}

			tech.shaderEntry_[type] = shader.entry;
			tech.shaderSource_[type] = pShaderSource;

			for (const auto& al : shader.aliases)
			{
				tech.aliases_.emplace_back(al);
			}
		}
	}

	return pTechDefState.release();
}

X_NAMESPACE_END