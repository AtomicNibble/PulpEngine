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
	shaderSource_.fill(nullptr);
}

TechDef::~TechDef()
{
	render::IRender* pRenderSys = gEnv->pRender;

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

	pTechDefs_ = X_NEW(TechSetDefs, g_3dEngineArena, "MatManTechSets")(g_3dEngineArena);

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

	for (auto idx = hashIndex_.first(hashVal); idx >= 0; idx = hashIndex_.next(idx))
	{
		auto* pTechDef = techStates_[idx];

		if (pTechDef->cat == cat && pTechDef->name == name)
		{
			return pTechDef;
		}
	}

	// not in cache.
	auto* pTechDef = loadTechDefState(cat, name);
	if (!pTechDef) {
		X_ERROR("TechDefState", "Failed to load tech state def: %s:%s", MaterialCat::ToString(cat), name.c_str());
		return nullptr;
	}

	// add to cache
	auto idx = techStates_.append(pTechDef);
	hashIndex_.insertIndex(hashVal, safe_static_cast<int32_t>(idx));

	return pTechDef;
}

TechDefState* TechDefStateManager::loadTechDefState(const MaterialCat::Enum cat, const core::string& name)
{
	TechSetDef* pTechDef = nullptr;
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

#if 0

bool TechDefStateManager::loadTechDefState(const MaterialCat::Enum cat, const core::string& name)
{
	TechSetDef* pTechDef = nullptr;
	if (!pTechDefs_->getTechDef(cat, name, pTechDef)) {
		X_ERROR("TechDefState", "Failed to get techdef definition for state creation");
		return false;
	}

	// ok so now we need to create state for this slut.
	// this involves render state and shaders.
	// a tech can have many techniques and we need to make state for each one.
	// but we need to support permatations for the techs.
	// so a given tech can be created supporting textures and not supporting textures.
	// which would results in changes to the shaders only?
	// do i want to manage peratation shit here?
	// i think it still makes sense to have that logic in here and not in materials.
	// since many materials will share the same techs and have same permatations.
	// so this class provides a way for materials to share them simular tech permatations.
	// 
	// Just need to figure out a nice way to handle this.
	// Since we only know what input layouts a shader supports after it's been processed.
	// which thinking about it is fine.
	// it's just other permatation shit that's got me thinking..
	// 
	// Ok since we define diffrent techs for each style of material like scroll and crap
	// materials will use diffrent techs if they want a diffrent permatation of the shader?
	// except not really, as we want to support diffrent streams for a given materials.
	// so if a material has selected a tech it still needs multiple permatations, one with all streams and another with reduced streams.
	// we also need to support the fact that maybe some of the optional maps have not been set for a techdef?
	// so that that is also a permatation?
	// 
	// ok so i think it's clear we need to store permatations for shaders.
	// for each tech def.
	// but we only store one shader since all permatations must come from the same source file.
	// 
	// so lets begin:
	// 1. for each tech def create a state.
	// 

	render::IRender* pRenderSys = gEnv->pRender;

	TechDefState* pTechDefState = X_NEW(TechDefState, arena_, "TechDef")(arena_);
	pTechDefState->cat = cat;
	pTechDefState->name = name;

	render::RenderTargetFmtsArr rtfs;
	rtfs.append(texture::Texturefmt::R8G8B8A8);

	auto passHandle = pRenderSys->createPassState(rtfs);
	if (passHandle == render::INVALID_STATE_HANLDE)
	{
		return false;
	}

	for (auto it = pTechDef->techBegin(); it != pTechDef->techEnd(); ++it)
	{
		const auto& techName = it->first;
		const auto& techDefTech = it->second;


		// so a tech def gives me a set of shaders to be used.
		// before i was getting the .shader files by name.
		// which i'm phasing out since they are not needed.
		// the only thing i need to decide on is how much to put in the shader system.
		// should the 3dengine be directly asking for source files and then asking for permatations of that?
		// seams like a resonable concept.
		// this way permatations can be data driven in some form by by the 3dengine.
		// so it's just a case of how flexible i want the api.
		// think i'll make it flexible and have the 3dengine direclty deal with source and permatations.
		// but the shader lib abstracts thar parsing of the techs and cbuffers and gives back nice neat uniformed data.
		// ....
		//
		// For a given tech we know what hardware stages are set (vertex,pixel,hull,..)
		// but currently the shader system abstracts that a bit.
		// well the render system needs the concept of permatation that defines all the shader stages so it knows how to bind them
		// so that kinda needs to stay the same.
		// we just need to work out how permatations are going to be created.
		// since the 3dengine is providing hat info now, the shader system can't just load it from a file.
		// like it was doing before.
		// lets flesh out how we want to use the api here.

		TechState& techState = pTechDefState->techs_.AddOne();
		techState.name = techName;

		render::shader::ShaderStagesArr stages{};

		for (uint32_t i = 0; i < render::shader::ShaderType::ENUM_COUNT - 1; i++)
		{
			const auto type = static_cast<render::shader::ShaderType::Enum>(i);
			const auto sage = staderTypeToStageFlag(type);

			if (!techDefTech.stages.IsSet(sage)) {
				continue;
			}

			const auto& shader = techDefTech.shaders[type];
			//  make sure the shader in the vertex slot is a vertex shader etc..
			X_ASSERT(shader.type == type, "Incorrect shader type for stage index.")(shader.type, type);

			// we need to get the shader source, so that we can create our permatations.
			// do we compile them now :Z ?
			// by doing this tho even when we have pre baked shaders we will still need source files :|
			// we would basically need to just pass shader source name instead of shader source, to createHWShader.
			// :'( 
			// the reason i'm parsing the shader source tho is to give me info about what input layouts it supports.
			// but if the engine is requesting that we are fucked anyway lol.
			// so many we could just move the checking of source flags when compiling from source.
			// and they just not used when baked shader is user.
			render::shader::IShaderSource* pShaderSource = pRenderSys->getShaderSource(shader.source.c_str());

			auto ilFlags = pShaderSource->getILFlags();

			// create a instance of the shader with the flags we want it compiled with.
			// this won't actually compile it.
			techState.pShaderSource[type] = pShaderSource;
			stages[type] = pRenderSys->createHWShader(shader.type, shader.entry, pShaderSource);
		}

		render::shader::IShaderPermatation* pPerm = pRenderSys->createPermatation(stages);


		// leave these blank for now.
		render::TextureState* pTexStates = nullptr;
		size_t numStates = 0;

		// after state creation the perm has all it's cbuffer info filled and and all HW shaders are compiled.
		auto stateHandle = pRenderSys->createState(passHandle, pPerm, techDefTech.state, pTexStates, numStates);

		if (stateHandle == render::INVALID_STATE_HANLDE)
		{
			return false;
		}


		techState.pPerm = pPerm;
		techState.stateHandle = stateHandle;

	}


	return true;
}

#endif

X_NAMESPACE_END