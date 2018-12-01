#include "stdafx.h"
#include "TechDefStateManager.h"

#include <Hashing\Fnva1Hash.h>
#include <Util\UniquePointer.h>

#include <Sampler.h>

#include <atomic>

X_NAMESPACE_BEGIN(engine)

namespace
{
    TechDefState* const INVALID_TECH_DEF_STATE = reinterpret_cast<TechDefState*>(std::numeric_limits<uintptr_t>::max());

} // namespace

TechDef::TechDef(const core::string& name, core::StrHash nameHash, const techset::TechSetDef* pTechSecDef, core::MemoryArenaBase* arena) :
    permArena_(arena),
    pTechSecDef_(pTechSecDef),
    perms_(arena),
    aliases_(arena),
    codeBinds_(arena),
    name_(name),
    nameHash_(nameHash)
{
    X_ASSERT(arena->isThreadSafe(), "Arena must be thread safe")();

    perms_.setGranularity(8);
}

TechDef::~TechDef()
{
    render::IRender* pRenderSys = gEnv->pRender;
    X_ASSERT_NOT_NULL(pRenderSys);

    for (auto* pPerm : perms_) {
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
        for (auto& pPerm : perms_) {
            if (pPerm->vertFmt == vertFmt && pPerm->permFlags == permFlags) {
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

        if (!pCompilingPerm) {
            notCompiled = true;

            // we don't have a perm, another thread might be compiling it tho.
            pCompilingPerm = X_NEW(TechDefPerm, permArena_, "TechDefPerm");
            pCompilingPerm->status = TechStatus::NOT_COMPILED;
            pCompilingPerm->permFlags = permFlags;
            pCompilingPerm->vertFmt = vertFmt;
            perms_.append(pCompilingPerm);
        }
        else {
            X_ASSERT(notCompiled == false, "Perm is already compiling")(notCompiled);
        }
    }

    // now we have left lock scope we wait for the perm to compile.
    // only if we are not the thread that is compiling it.
    if (!notCompiled) {
        while (pCompilingPerm->status == TechStatus::NOT_COMPILED) {
            // TODO: something useful.
            core::Thread::yield();
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

    for (size_t i = 0; i < shaderSource_.size(); i++) {
        auto& source = shaderSource_[i];
        if (source.isEmpty()) {
            continue;
        }

        const auto type = static_cast<render::shader::ShaderType::Enum>(i);

        // create a instance of the shader with the flags we want it compiled with.
        // this won't actually compile it.
        stages[type] = pRenderSys->createHWShader(type, shaderEntry_[type], shaderDefines_[type], source, permFlags, vertFmt);
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
    if (passHandle == render::INVALID_STATE_HANLDE) {
        X_ERROR("Tech", "Failed to create passState");
        pRenderSys->releaseShaderPermatation(pPerm);
        pCompilingPerm->status = TechStatus::ERROR;
        return false;
    }

    // make a copy of the state and alter certain states.
    // anything that is actually defined in the techdef's should not be overriden here.
    // only stuff like vertex format which is runtime etc..
    decltype(stateDesc_) stateDescCpy = stateDesc_;
    stateDescCpy.vertexFmt = vertFmt;

    // come up with better way to send this data to render?
    // or does it just make sense to be part of state..?
    if (permFlags.IsSet(PermatationFlags::VertStreams)) {
        stateDescCpy.stateFlags.Set(render::StateFlag::VERTEX_STREAMS);
    }
    if (permFlags.IsSet(PermatationFlags::Instanced)) {
        stateDescCpy.stateFlags.Set(render::StateFlag::INSTANCED_POS_COLOR);
    }
    if (permFlags.IsSet(PermatationFlags::HwSkin)) {
        stateDescCpy.stateFlags.Set(render::StateFlag::HWSKIN);
    }

    core::FixedArray<render::SamplerState, render::MAX_SAMPLERS_BOUND> staticSamplers;

    // can i work out if this perm is static samplers only.
    // TODO: suppport aliasing.
    {
        auto& permSamplers = pPerm->getSamplers();
        if (permSamplers.isNotEmpty() && pTechSecDef_->anySamplersAreStatic()) {
            auto samplers = pTechSecDef_->getSamplers();

            bool allStatic = pTechSecDef_->allSamplersAreStatic();

            // see if all the ones we use are static.
            if (!allStatic) {
                allStatic = true;

                for (auto& ps : permSamplers) {
                    auto it = std::find_if(samplers.begin(), samplers.end(),
                        [&ps](const decltype(samplers)::element_type& s) -> bool {
                            return s.second.isStatic() && s.first == ps.getName();
                        });

                    allStatic &= (it != samplers.end());
                }
            }

            if (allStatic) {
                // map them.
                for (auto& ps : permSamplers) {
                    auto it = std::find_if(samplers.begin(), samplers.end(),
                        [&ps](const decltype(samplers)::element_type& s) -> bool {
                            return s.first == ps.getName();
                        });

                    if (it == samplers.end()) {
                        X_ERROR("Tech", "Failed to map static samplers.");
                        staticSamplers.clear();
                        break;
                    }

                    X_ASSERT(it->second.isStatic(), "Should be static sampler")(it->first, it->second.isStatic());

                    render::SamplerState ss;
                    ss.filter = it->second.filter;
                    ss.repeat = it->second.repeat;
                    staticSamplers.push_back(ss);
                }
            }
        }
    }

    auto stateHandle = pRenderSys->createState(
        passHandle,
        pPerm,
        stateDescCpy,
        staticSamplers.begin(), staticSamplers.size());

    if (stateHandle == render::INVALID_STATE_HANLDE) {
        X_BREAKPOINT;
        X_ERROR("Tech", "Failed to create state");
        pRenderSys->destoryPassState(passHandle);
        pRenderSys->releaseShaderPermatation(pPerm);
        pCompilingPerm->status = TechStatus::ERROR;
        return false;
    }

    // work out variable state size.
    {
        const auto& cbLinks = pPerm->getCbufferLinks();
        const auto& buffers = pPerm->getBuffers();
        const auto& permSamplers = pPerm->getSamplers();
        const auto& permTextures = pPerm->getTextures();

        size_t numSamplers = permSamplers.size();

        if (pTechSecDef_->allSamplersAreStatic()) {
            numSamplers = 0;
        }

        auto size = render::Commands::ResourceStateBase::getStateSize(
            permTextures.size(),
            numSamplers,
            cbLinks.size(),
            buffers.size());

        pCompilingPerm->variableStateSize = safe_static_cast<int8_t>(size);

        pCompilingPerm->numTextStates = safe_static_cast<int8_t>(permTextures.size());
        pCompilingPerm->numSamplers = safe_static_cast<int8_t>(numSamplers);
        pCompilingPerm->numCbs = safe_static_cast<int8_t>(cbLinks.size());
        pCompilingPerm->numBuffers = safe_static_cast<int8_t>(buffers.size());
    }

    pCompilingPerm->pTechDef = this;
    pCompilingPerm->stateHandle = stateHandle;
    pCompilingPerm->pShaderPerm = pPerm;
    COMPILER_BARRIER_W;
    pCompilingPerm->status = TechStatus::COMPILED; // any other threads waiting on this can now return.
    return pCompilingPerm;
}

// -----------------------------------------------------------------------

TechDefState::TechDefState(MaterialCat::Enum cat, const core::string& name, core::MemoryArenaBase* arena) :
    cat_(cat),
    name_(name),
    techs_(arena),
    pTechSecDef_(nullptr)
{
    X_ASSERT(arena->isThreadSafe(), "Arena must be thread safe")();

    techs_.setGranularity(1);
}

TechDefState::~TechDefState()
{
}

TechDef* TechDefState::getTech(core::StrHash hash)
{
    for (auto& tech : techs_) {
        if (tech.nameHash_ == hash) {
            return &tech;
        }
    }

    return nullptr;
}

// -----------------------------------------------------------------------

TechDefStateManager::TechDefStateManager(core::MemoryArenaBase* arena) :
    arena_(arena),
    techs_(arena, TECH_DEFS_MAX),
    techsPoolHeap_(
        core::bitUtil::RoundUpToMultiple<size_t>(
            PoolArena::getMemoryRequirement(sizeof(TechDefState)) * TECH_DEFS_MAX,
            core::VirtualMem::GetPageSize())),
    techsPoolAllocator_(techsPoolHeap_.start(), techsPoolHeap_.end(),
        PoolArena::getMemoryRequirement(sizeof(TechDefState)),
        PoolArena::getMemoryAlignmentRequirement(X_ALIGN_OF(TechDefState)),
        PoolArena::getMemoryOffsetRequirement()),
    techsPoolArena_(&techsPoolAllocator_, "TechDefStatePool")
{
    arena->addChildArena(&techsPoolArena_);

    pTechDefs_ = X_NEW(techset::TechSetDefs, arena, "MatManTechSets")(arena);
}

TechDefStateManager::~TechDefStateManager()
{
    shutDown();
}

void TechDefStateManager::shutDown(void)
{
    core::CriticalSection::ScopedLock lock(cacheLock_);

    for (auto& it : techs_) {

        if (it.second == INVALID_TECH_DEF_STATE) {
            X_WARNING("TechDefState", "Skipping delete of invalid tech state: \"%s\"", it.first.second.c_str());
            continue;
        }

        X_DELETE(it.second, &techsPoolArena_);
    }

    techs_.free();

    if (pTechDefs_) {
        X_DELETE_AND_NULL(pTechDefs_, g_3dEngineArena);
    }
}

TechDefState* TechDefStateManager::getTechDefState(const MaterialCat::Enum cat, const core::string& name)
{
    TechCatNamePair pair(cat, name);

    TechDefState** pTechDefRef = nullptr;
    bool loaded = true;
    {
        core::CriticalSection::ScopedLock lock(cacheLock_);

        auto it = techs_.find(pair);
        if (it != techs_.end()) {
            pTechDefRef = &it->second;
        }
        else {
            loaded = false;
            auto insertIt = techs_.insert(std::make_pair(pair, nullptr));

            pTechDefRef = &insertIt.first->second;
        }
    }

    if (loaded) {
        while (*pTechDefRef == nullptr) {
            core::Thread::yield();
        }

        if (*pTechDefRef == INVALID_TECH_DEF_STATE) {
            return nullptr;
        }

        return *pTechDefRef;
    }

    // not in cache.
    auto* pTechDef = loadTechDefState(cat, name);
    if (!pTechDef) {
        X_ERROR("TechDefState", "Failed to load tech state def: %s:%s", MaterialCat::ToString(cat), name.c_str());
        *pTechDefRef = INVALID_TECH_DEF_STATE;
        return nullptr;
    }

    *pTechDefRef = pTechDef;
    return pTechDef;
}

TechDefState* TechDefStateManager::loadTechDefState(const MaterialCat::Enum cat, const core::string& name)
{
    X_ASSERT(arena_->isThreadSafe(), "Arena must be thread safe")();
    static_assert(decltype(techsPoolArena_)::IS_THREAD_SAFE, "Arena must be thread safe");

    auto* pTechDef = pTechDefs_->getTechDef(cat, name);
    if (!pTechDef) {
        X_ERROR("TechDefState", "Failed to get techdef definition for state creation");
        return nullptr;
    }

    // this don't cause any state to be created.
    // permatations are created on demand.
    core::UniquePointer<TechDefState> pTechDefState = core::makeUnique<TechDefState>(&techsPoolArena_, cat, name, arena_);
    pTechDefState->techs_.reserve(pTechDef->numTechs());
    pTechDefState->pTechSecDef_ = pTechDef;

    // we process all the techs
    for (auto it : pTechDef->getTechs()) {
        const auto& techName = it.first;
        const auto& techDefTech = it.second;

        auto& tech = pTechDefState->techs_.AddOne(techName, core::StrHash(techName.c_str(), techName.length()), pTechDef, arena_);
        tech.stateDesc_ = techDefTech.state;

        for (uint32_t i = 0; i < render::shader::ShaderType::ENUM_COUNT - 1; i++) {
            const auto type = static_cast<render::shader::ShaderType::Enum>(i);
            const auto stage = staderTypeToStageFlag(type);

            if (!techDefTech.stages.IsSet(stage)) {
                continue;
            }

            const auto& shader = techDefTech.shaders[type];

            //  make sure the shader in the vertex slot is a vertex shader etc..
            X_ASSERT(shader.type == type, "Incorrect shader type for stage index.")(shader.type, type);

            tech.shaderSource_[type] = shader.source;
            tech.shaderDefines_[type] = shader.defines;
            tech.shaderEntry_[type] = shader.entry;

            for (const auto& al : shader.aliases) {
                tech.aliases_.emplace_back(al);
            }

            for (const auto& cb : shader.codeBinds) {
                tech.codeBinds_.emplace_back(cb);
            }
        }
    }

    return pTechDefState.release();
}

X_NAMESPACE_END