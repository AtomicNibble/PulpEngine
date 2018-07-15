#include "stdafx.h"
#include "X3DEngine.h"

#include <ITimer.h>
#include <IRender.h>
#include <IConsole.h>
#include <IGui.h>
#include <IPhysics.h>
#include <CBuffer.h>
#include <IFrameData.h>
#include <IFont.h>
#include <IVideo.h>

#include <Time\StopWatch.h>
#include <String\StringHash.h>
#include <Random\XorShift.h>

#include "Material\MaterialManager.h"
#include "Texture\TextureManager.h"
#include "Model\ModelManager.h"
#include "Anim\AnimManager.h"
#include "Particles\FxManager.h"
#include "Gui\GuiManger.h"

#include "CmdBucket.h"

#include "Drawing\CBufferManager.h"
#include "Drawing\VariableStateManager.h"

#include "World\World3D.h"

X_NAMESPACE_BEGIN(engine)

EngineEnv gEngEnv;

//------------------------------------------

X3DEngine::X3DEngine(core::MemoryArenaBase* arena) :
    pMaterialManager_(nullptr),
    pTextureManager_(nullptr),
    pModelManager_(nullptr),
    pAnimManager_(nullptr),
    pEffectManager_(nullptr),
    pGuiManger_(nullptr),
    pCBufMan_(nullptr),
    pVariableStateMan_(nullptr),
    primContexts_{
        {primResources_, IPrimativeContext::Mode::Mode3D, arena}, // sound
        {primResources_, IPrimativeContext::Mode::Mode3D, arena}, // physics
        {primResources_, IPrimativeContext::Mode::Mode3D, arena}, // misc3d
        {primResources_, IPrimativeContext::Mode::Mode3D, arena}, // persistent
        {primResources_, IPrimativeContext::Mode::Mode2D, arena}, // gui
        {primResources_, IPrimativeContext::Mode::Mode2D, arena}, // profile
        {primResources_, IPrimativeContext::Mode::Mode2D, arena}  // console
    },
    clearPersistent_(false),
    worlds_(arena)
{
    // check if the enum order was changed in a way that resulted in incorrect modes.
    X_ASSERT(primContexts_[PrimContext::SOUND].getMode() == IPrimativeContext::Mode::Mode3D, "Incorrect mode")();
    X_ASSERT(primContexts_[PrimContext::PHYSICS].getMode() == IPrimativeContext::Mode::Mode3D, "Incorrect mode")();
    X_ASSERT(primContexts_[PrimContext::MISC3D].getMode() == IPrimativeContext::Mode::Mode3D, "Incorrect mode")();
    X_ASSERT(primContexts_[PrimContext::PERSISTENT].getMode() == IPrimativeContext::Mode::Mode3D, "Incorrect mode")();
    X_ASSERT(primContexts_[PrimContext::GUI].getMode() == IPrimativeContext::Mode::Mode2D, "Incorrect mode")();
    X_ASSERT(primContexts_[PrimContext::PROFILE].getMode() == IPrimativeContext::Mode::Mode2D, "Incorrect mode")();
    X_ASSERT(primContexts_[PrimContext::CONSOLE].getMode() == IPrimativeContext::Mode::Mode2D, "Incorrect mode")();
}

X3DEngine::~X3DEngine()
{
}

void X3DEngine::registerVars(void)
{
    drawVars_.registerVars();
}

void X3DEngine::registerCmds(void)
{
    ADD_COMMAND_MEMBER("r_clear_persistent", this, X3DEngine, &X3DEngine::Command_ClearPersistent,
        core::VarFlag::SYSTEM, "Clears persistent primatives");
}

render::IPixelBuffer* pDepthStencil = nullptr;

bool X3DEngine::init(void)
{
    X_PROFILE_NO_HISTORY_BEGIN("3DEngineInit", core::profiler::SubSys::ENGINE3D);

    if (gEngEnv.p3DEngine_) {
        return false;
    }

    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pCore);
    X_ASSERT_NOT_NULL(gEnv->pTimer);
    X_ASSERT_NOT_NULL(gEnv->pFileSys);
    X_ASSERT_NOT_NULL(gEnv->pConsole);
    X_ASSERT_NOT_NULL(gEnv->pRender);
    X_ASSERT_NOT_NULL(gEnv->pHotReload);
    X_ASSERT_NOT_NULL(gEnv->pPhysics);

    auto* pRender = gEnv->pRender;

    // register some file types.
    gEnv->pHotReload->addfileType(this, "level");
    gEnv->pHotReload->addfileType(this, "map");

    pCBufMan_ = X_NEW(CBufferManager, g_3dEngineArena, "CBufMan")(g_3dEngineArena, pRender);
    pVariableStateMan_ = X_NEW(VariableStateManager, g_3dEngineArena, "StateMan");
    pTextureManager_ = X_NEW(engine::TextureManager, g_3dEngineArena, "TextureManager")(g_3dEngineArena);
    pMaterialManager_ = X_NEW(engine::XMaterialManager, g_3dEngineArena, "MaterialManager")(g_3dEngineArena, *pVariableStateMan_, *pCBufMan_);
    pModelManager_ = X_NEW(model::XModelManager, g_3dEngineArena, "ModelManager")(g_3dEngineArena, g_3dEngineArena);
    pAnimManager_ = X_NEW(anim::AnimManager, g_3dEngineArena, "AnimManager")(g_3dEngineArena, g_3dEngineArena);
    pEffectManager_ = X_NEW(fx::EffectManager, g_3dEngineArena, "EffectManager")(g_3dEngineArena, g_3dEngineArena);
    pGuiManger_ = X_NEW(gui::XGuiManager, g_3dEngineArena, "GuiManager")(g_3dEngineArena, pMaterialManager_);

    pTextureManager_->registerCmds();
    pTextureManager_->registerVars();
    pMaterialManager_->registerCmds();
    pMaterialManager_->registerVars();
    pModelManager_->registerCmds();
    pModelManager_->registerVars();
    pAnimManager_->registerCmds();
    pAnimManager_->registerVars();
    pEffectManager_->registerCmds();
    pEffectManager_->registerVars();

    gEngEnv.pGuiMan_ = pGuiManger_;
    gEngEnv.pMaterialMan_ = pMaterialManager_;
    gEngEnv.pTextureMan_ = pTextureManager_;
    gEngEnv.pModelMan_ = pModelManager_;
    gEngEnv.pEffectMan_ = pEffectManager_;
    gEngEnv.p3DEngine_ = this;

    {
        X_PROFILE_NO_HISTORY_BEGIN("3DManInit", core::profiler::SubSys::ENGINE3D);

        if (!pTextureManager_->init()) {
            return false;
        }
        if (!pMaterialManager_->init()) {
            return false;
        }
        if (!pModelManager_->init()) {
            return false;
        }
        if (!pAnimManager_->init()) {
            return false;
        }
        if (!pEffectManager_->init()) {
            return false;
        }
        if (!pGuiManger_->init()) {
            return false;
        }
    }

    if (!primResources_.init(pRender, pMaterialManager_)) {
        X_ERROR("3DEngine", "Failed to init prim resources");
        return false;
    }

    auto* pPhysics = gEnv->pPhysics;

    // init physics.
    // this don't create a scene, that's done later..
    physics::ToleranceScale scale;
    scale.length = physics::SCALE_LENGTH;
    scale.mass = physics::SCALE_MASS;
    scale.speed = physics::SCALE_SPEED;

    if (!pPhysics->init(scale)) {
        X_ERROR("3DEngine", "Failed to setup physics scene");
        return false;
    }

    // setup groups collision filters.
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::AiClip, physics::GroupFlag::Player, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::AiClip, physics::GroupFlag::Ai, true);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::AiClip, physics::GroupFlag::Vehicle, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::PlayerClip, physics::GroupFlag::Player, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::PlayerClip, physics::GroupFlag::Ai, true);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::PlayerClip, physics::GroupFlag::Vehicle, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::VehicleClip, physics::GroupFlag::Player, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::VehicleClip, physics::GroupFlag::Ai, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::VehicleClip, physics::GroupFlag::Vehicle, true);

    pDepthStencil = pRender->createDepthBuffer("$depth_buffer", Vec2i(1680, 1050));

    return true;
}

void X3DEngine::shutDown(void)
{
    X_LOG0("3DEngine", "Shutting Down");

    gEnv->pHotReload->addfileType(nullptr, "level");
    gEnv->pHotReload->addfileType(nullptr, "map");

    auto* pRender = gEnv->pRender;

    for (auto& primcon : primContexts_) {
        primcon.freePages(pRender);
    }

    primResources_.releaseResources(pRender, pMaterialManager_);

    if (pGuiManger_) {
        pGuiManger_->shutdown();
        X_DELETE(pGuiManger_, g_3dEngineArena);

        gEngEnv.pGuiMan_ = nullptr;
    }

    if (pEffectManager_) {
        pEffectManager_->shutDown();
        X_DELETE(pEffectManager_, g_3dEngineArena);

        gEngEnv.pEffectMan_ = nullptr;
    }

    if (pAnimManager_) {
        pAnimManager_->shutDown();
        X_DELETE(pAnimManager_, g_3dEngineArena);

        // ...
    }

    if (pModelManager_) {
        pModelManager_->shutDown();
        X_DELETE(pModelManager_, g_3dEngineArena);

        gEngEnv.pModelMan_ = nullptr;
    }

    if (pMaterialManager_) {
        pMaterialManager_->shutDown();
        X_DELETE(pMaterialManager_, g_3dEngineArena);

        gEngEnv.pMaterialMan_ = nullptr;
    }

    if (pTextureManager_) {
        pTextureManager_->shutDown();
        X_DELETE(pTextureManager_, g_3dEngineArena);

        gEngEnv.pTextureMan_ = nullptr;
    }

    if (pVariableStateMan_) {
        pVariableStateMan_->shutDown();
        X_DELETE(pVariableStateMan_, g_3dEngineArena);
    }

    if (pCBufMan_) {
        pCBufMan_->shutDown();
        X_DELETE(pCBufMan_, g_3dEngineArena);
    }

    gEngEnv.p3DEngine_ = nullptr;
}

void X3DEngine::release(void)
{
    X_DELETE(this, g_3dEngineArena);
}

bool X3DEngine::asyncInitFinalize(void)
{
    bool allOk = true;

    // all init okay?
    // we must allways call asyncInitFinalize on all instances.
    // so shutdown is safe.

    if (!pTextureManager_ || !pTextureManager_->asyncInitFinalize()) {
        allOk = false;
    }

    if (!pMaterialManager_ || !pMaterialManager_->asyncInitFinalize()) {
        allOk = false;
    }

    if (!pModelManager_ || !pModelManager_->asyncInitFinalize()) {
        allOk = false;
    }

    if (!pAnimManager_ || !pAnimManager_->asyncInitFinalize()) {
        allOk = false;
    }

    return allOk;
}

void X3DEngine::Update(core::FrameData& frame)
{
    X_PROFILE_BEGIN("3DUpdate", core::profiler::SubSys::ENGINE3D);

    pTextureManager_->scheduleStreaming();

    //	Matrix44f view = Matrix44f::identity();
    //	Matrix44f viewProj;
    //	MatrixOrthoOffCenterRH(&viewProj, 0, 1680, 1050, 0, -1e10f, 1e10);

    X_UNUSED(frame);
    //	pCBufMan_->update(frame, false);
}

void X3DEngine::OnFrameBegin(core::FrameData& frame)
{
    X_PROFILE_BEGIN("3DFrameBegin", core::profiler::SubSys::ENGINE3D);

    auto* pRender = gEnv->pRender;

#if 0
	XCamera cam;
	CmdPacketAllocator cmdBucketAllocator(g_3dEngineArena, 0x4000 * 64);

	cmdBucketAllocator.createAllocaotrsForThreads(*gEnv->pJobSys);

	CommandBucket<uint16_t> testBucket(g_3dEngineArena, cmdBucketAllocator, 0x4000, cam);

	core::StopWatch timer;

	for (size_t i = 0; i < 0x4000; i++)
	{
		render::Commands::Draw* pDraw = testBucket.addCommand<render::Commands::Draw>(13, 0);
		pDraw->indexBuffer = 0;
		pDraw->vertexBuffer = 0;
	}

	const core::TimeVal addTime = timer.GetTimeVal();
	timer.Start();

	testBucket.sort();

	const core::TimeVal sortTime = timer.GetTimeVal();

	X_LOG0("test", "addTime: %fms", addTime.GetMilliSeconds());
	X_LOG0("test", "sortTime: %fms", sortTime.GetMilliSeconds());

	testBucket.submit();
#endif

#if 1
    {
        XCamera cam;
        XViewPort viewPort = frame.view.viewport;

        render::CmdPacketAllocator cmdBucketAllocator(g_3dEngineArena, 8096 * 12);
        cmdBucketAllocator.createAllocaotrsForThreads(*gEnv->pJobSys);
        render::CommandBucket<uint32_t> geoBucket(g_3dEngineArena, cmdBucketAllocator, 8096 * 5, cam, viewPort);

        geoBucket.appendRenderTarget(pRender->getCurBackBuffer());
        geoBucket.setDepthStencil(pDepthStencil, render::DepthBindFlag::CLEAR | render::DepthBindFlag::WRITE);

        if (pCBufMan_->update(frame, false)) {
            render::Commands::Nop* pNop = geoBucket.addCommand<render::Commands::Nop>(0, 0);

            pCBufMan_->updatePerFrameCBs(geoBucket, pNop);
        }

        for (auto* pWorld : worlds_) {
            static_cast<World3D*>(pWorld)->renderView(frame, geoBucket);
        }

        if (drawVars_.drawDepth()) {
            TechDefPerm* pTech = pMaterialManager_->getCodeTech(
                core::string("fullscreen_depth"),
                core::StrHash("unlit"),
                render::shader::VertexFormat::NONE,
                PermatationFlags::Textured);

            auto* pDraw = geoBucket.addCommand<render::Commands::Draw>(0xffffff, pTech->variableStateSize);

            pDraw->startVertex = 0;
            pDraw->vertexCount = 3;
            pDraw->stateHandle = pTech->stateHandle;
            core::zero_object(pDraw->vertexBuffers);
            core::zero_object(pDraw->resourceState);

            if (pTech->variableStateSize) {
                RegisterCtx ctx;
                ctx.regs[Register::CodeTexture0] = pDepthStencil->getTexID();

                pMaterialManager_->initStateFromRegisters(pTech, &pDraw->resourceState, ctx);
            }
        }

        geoBucket.sort();

        pRender->submitCommandPackets(geoBucket);
    }

#else
    {
        // we need a buffer for the depth.
        //	render::IPixelBuffer* pDepthStencil = pRender_->createPixelBuffer("$depth_buffer", Vec2i(1680, 1050),
        //		texture::Texturefmt::R24G8_TYPELESS, render::PixelBufferType::DEPTH);

        XCamera cam;
        XViewPort viewPort = frame.view.viewport;

        render::CmdPacketAllocator cmdBucketAllocator(g_3dEngineArena, 8096 * 12);
        cmdBucketAllocator.createAllocaotrsForThreads(*gEnv->pJobSys);
        render::CommandBucket<uint32_t> geoBucket(g_3dEngineArena, cmdBucketAllocator, 8096 * 5, cam, viewPort);

        geoBucket.appendRenderTarget(pRender->getCurBackBuffer());
        geoBucket.setDepthStencil(pDepthStencil, render::DepthBindFlag::CLEAR | render::DepthBindFlag::WRITE);

        if (pCBufMan_->update(frame, false)) {
            render::Commands::Nop* pNop = geoBucket.addCommand<render::Commands::Nop>(0, 0);

            pCBufMan_->updatePerFrameCBs(geoBucket, pNop);
        }

        //	pCBufMan_->update(frame, false);

        level_.dispatchJobs(frame, geoBucket);

        geoBucket.sort();

        pRender->submitCommandPackets(geoBucket);
    }
#endif

    for (auto* pWorld : worlds_) {
        static_cast<World3D*>(pWorld)->renderEmitters(frame, &primContexts_[PrimContext::MISC3D]);
    }
    // ok so lets dump out the primative context.
    // I will need to have some sort of known time that it's
    // safe to start creating gpu commands for the context.
    // since te

    // we could populate this command bucket with a job for each primContext.
    // and just shove the index at MSB of key to keep order.
    size_t totalElems = 0;

    for (uint16_t i = 0; i < engine::PrimContext::ENUM_COUNT; i++) {
        const auto& context = primContexts_[i];
        if (!context.isEmpty()) {
            const auto& elems = context.getUnsortedBuffer();
            const auto& objBufs = context.getShapeArrayBuffers();

            totalElems += elems.size();

            for (auto& objBuf : objBufs) {
                totalElems += objBuf.size();
            }
        }
    }

    if (totalElems > 0) {
        XCamera cam;
        XViewPort viewPort = frame.view.viewport;

        render::CmdPacketAllocator cmdBucketAllocator(g_3dEngineArena, totalElems * 1024);
        cmdBucketAllocator.createAllocaotrsForThreads(*gEnv->pJobSys);
        render::CommandBucket<uint32_t> primBucket(g_3dEngineArena, cmdBucketAllocator, totalElems * 4, cam, viewPort);

        primBucket.appendRenderTarget(pRender->getCurBackBuffer());
        primBucket.setDepthStencil(pDepthStencil, render::DepthBindFlag::WRITE);

        // the updating of dirty font buffers should happen regardless of
        // prim drawing.
        // unless only way to draw text is with prim humm.... !
        gEnv->pFontSys->appendDirtyBuffers(primBucket);
        gEnv->pVideoSys->appendDirtyBuffers(primBucket);

        for (uint16_t i = 0; i < engine::PrimContext::ENUM_COUNT; i++) {
            auto& context = primContexts_[i];
            if (!context.isEmpty()) {
                const bool is2d = context.getMode() == IPrimativeContext::Mode::Mode2D;

                const uint32_t primSortKeyBase = i * 16384;

                if (pCBufMan_->update(frame, is2d)) {
                    render::Commands::Nop* pNop = primBucket.addCommand<render::Commands::Nop>(primSortKeyBase, 0);

                    pCBufMan_->updatePerFrameCBs(primBucket, pNop);
                }

                // create a command(s) to update the VB data.
                context.appendDirtyBuffers(primBucket);

                const auto& elems = context.getUnsortedBuffer();
                auto vertexPageHandles = context.getVertBufHandles();

                for (size_t x = 0; x < elems.size(); x++) {
                    const auto& elem = elems[x];
                    Material* pMat = elem.material;
                    uintptr_t pageIdx = elem.material.GetBits();

                    // so when we have a material we need select a tech and a permatation.
                    // then we will have a state handle and variable state.
                    // so we will want to select the tech we are rendering with.
                    const core::StrHash tech("unlit");

                    // i want a tech for this material to render with.
                    // if i'm supporting diffrent vertex formats.
                    // when the 3dengine decides it wants to render something instanced it need to be able to pick that.
                    // i don't want to make the manual techs so we pass tech name and permatations is selected from that.
                    // but should a material have all perms even if not using or should it make them on demand.
                    // if we can move perms into techStateDefs that would be nicer.
                    // since then we don't care to much about creating them all.
                    // so if this was able to create variable states and perms on demand all would be good in the world.
                    // as this then basically magically gives us multithread state creation..
                    // but fuck... if some states are taking over two frames to compile we need to really just not draw till that state is ready.
                    // so maybe if we ask mat man for states for each material.
                    // this way once a material has the states it will just return it's local copy.
                    // but if the state needs to be created we can create a job todo it.
                    // is it better to stall or just not render till next frame..
                    // well we can make it just not draw and add option to stall on request if needs be.
                    // lets just get it functional and see what is causing most of the delays.
                    const auto* pTech = pMaterialManager_->getTechForMaterial(pMat, tech, IPrimativeContext::VERTEX_FMT);

                    const auto* pPerm = pTech->pPerm;
                    const auto stateHandle = pPerm->stateHandle;
                    const auto* pVariableState = pTech->pVariableState;
                    auto variableStateSize = pVariableState->getStateSize();

                    // we must update cb's BEFORE adding a draw command.
                    if (variableStateSize) {
#if 0
						const auto numCBs = pTech->pVariableState->getNumCBs();
						if (numCBs)
						{
							const auto* pCBHandles = pTech->pVariableState->getCBs();
							const auto& cbLinks = pPerm->pShaderPerm->getCbufferLinks();

							for (int8_t j = 0; j < numCBs; j++)
							{
								auto& cbLink = cbLinks[j];
								auto& cb = *cbLink.pCBufer;

								if (!cb.requireManualUpdate())
								{
									// might as well provide intial data.
									if (pCBufMan_->autoUpdateBuffer(cb))
									{
										auto* pCBufUpdate = primBucket.addCommand<render::Commands::CopyConstantBufferData>(
											static_cast<uint32_t>((i * 500) + x + 1 + j),
											cb.getBindSize()
											);

										char* pAuxData = render::CommandPacket::getAuxiliaryMemory(pCBufUpdate);
										std::memcpy(pAuxData, cb.getCpuData().data(), cb.getBindSize());

										pCBufUpdate->constantBuffer = pCBHandles[j];
										pCBufUpdate->pData = pAuxData; // cb.getCpuData().data();
										pCBufUpdate->size = cb.getBindSize();
									}
								}
							}
						}
#endif
                    }

                    render::Commands::Draw* pDraw = primBucket.addCommand<render::Commands::Draw>(primSortKeyBase + static_cast<uint32_t>(x), variableStateSize);
                    pDraw->startVertex = elem.vertexOffs;
                    pDraw->vertexCount = elem.numVertices;
                    pDraw->stateHandle = stateHandle;
                    pDraw->resourceState = *pVariableState; // slice the sizes into command.
                    // set the vertex handle to correct one.
                    core::zero_object(pDraw->vertexBuffers);
                    pDraw->vertexBuffers[VertexStream::VERT] = vertexPageHandles[pageIdx];

                    if (variableStateSize) {
                        // variable state data.
                        char* pAuxData = render::CommandPacket::getAuxiliaryMemory(pDraw);
                        std::memcpy(pAuxData, pVariableState->getDataStart(), pVariableState->getStateSize());
                    }
                }

                // shapes
                if (!context.hasShapeData()) {
                    continue;
                }

                // shapeData
                //	this code is a little elabrate, but makes drawing primative shapes pretty cheap.
                //	it copyies all the instance data for all shapes for all lod's to a series of instance data pages.
                //	all the buffer copying is done before any drawing to give gpu some time to copy instance data
                //	then all the shapes are drawn with instance calls, meaning drawing 1000 spheres of variang sizes and color
                //	will result in about 4 page data copyies followed by 4 draw calls.
                //  which is kinda nice as the first draw call can begin before the instance data for the other 3 have finished
                {
                    // what material to draw with?
                    Material* pMat = primResources_.getMaterial(render::TopoType::TRIANGLELIST);

                    auto& objShapeLodBufs = context.getShapeArrayBuffers();

                    // sort them all. in parrell if you like,
                    for (size_t x = 0; x < objShapeLodBufs.size(); x++) {
                        objShapeLodBufs[x].sort();
                    }

                    auto& instPages = primResources_.getInstancePages();
                    const uint32_t maxPerPage = PrimativeContextSharedResources::NUM_INSTANCE_PER_PAGE;

                    render::Commands::CopyVertexBufferData* pBufUpdateCommand = nullptr;

                    // buffer copy commands.
                    {
                        // we need to keep trac of space left, so we can merge all shapes into single inst pages.
                        uint32_t curPageSpace = maxPerPage;
                        size_t curInstPage = 0;

                        if (!instPages[curInstPage].isVbValid()) {
                            instPages[curInstPage].createVB(pRender);
                        }

                        for (size_t x = 0; x < objShapeLodBufs.size(); x++) {
                            PrimativeContext::ShapeType::Enum shapeType = static_cast<PrimativeContext::ShapeType::Enum>(x);
                            auto& shapeBuf = objShapeLodBufs[x];

                            if (shapeBuf.isEmpty()) {
                                continue;
                            }

                            uint32_t numInstLeft = safe_static_cast<uint32_t>(shapeBuf.size());
                            auto& instData = shapeBuf.getData();
                            auto* pInstData = instData.data();

                            while (numInstLeft > 0 && curInstPage < instPages.size()) {
                                if (curPageSpace == 0) {
                                    ++curInstPage;

                                    if (curInstPage >= instPages.size()) {
                                        continue;
                                    }

                                    // make page buffer if required.
                                    auto& instPage = instPages[curInstPage];
                                    if (!instPage.isVbValid()) {
                                        instPage.createVB(pRender);
                                    }

                                    curPageSpace = maxPerPage;
                                }

                                uint32_t batchSize = core::Min(numInstLeft, curPageSpace);
                                const uint32_t batchOffset = maxPerPage - curPageSpace;

                                numInstLeft -= batchSize;
                                curPageSpace -= batchSize;

                                const auto& instPage = instPages[curInstPage];

                                render::Commands::CopyVertexBufferData* pUpdateBuf = nullptr;
                                if (!pBufUpdateCommand) {
                                    // move all the buffer updates to the start?
                                    pUpdateBuf = primBucket.addCommand<render::Commands::CopyVertexBufferData>(primSortKeyBase, 0);
                                }
                                else {
                                    pUpdateBuf = primBucket.appendCommand<render::Commands::CopyVertexBufferData>(pBufUpdateCommand, 0);
                                }
                                pUpdateBuf->vertexBuffer = instPage.instBufHandle;
                                pUpdateBuf->pData = pInstData;
                                pUpdateBuf->size = batchSize * sizeof(IPrimativeContext::ShapeInstanceData);
                                pUpdateBuf->dstOffset = batchOffset * sizeof(IPrimativeContext::ShapeInstanceData);

                                // chain them
                                pBufUpdateCommand = pUpdateBuf;

                                pInstData += batchSize;
                            }

                            // can we draw them all?
                            if (numInstLeft > 0) {
                                X_WARNING("Engine", "Not enougth pages to draw all primative %s's. total: %" PRIuS " dropped: %" PRIu32,
                                    IPrimativeContext::ShapeType::ToString(shapeType), instData.size(), numInstLeft);
                            }
                        }
                    }

                    // now we draw all shapes.
                    {
                        uint32_t curPageSpace = maxPerPage;
                        size_t curInstPage = 0;

                        for (size_t x = 0; x < objShapeLodBufs.size(); x++) {
                            PrimativeContext::ShapeType::Enum shapeType = static_cast<PrimativeContext::ShapeType::Enum>(x);
                            const auto& shapeRes = context.getShapeResources(shapeType);
                            auto& shapeBuf = objShapeLodBufs[x];
                            auto& shapeCnts = shapeBuf.getShapeCounts();

                            for (size_t solidIdx = 0; solidIdx < shapeCnts.size(); ++solidIdx) {
                                // index 1 is solid shapes.
                                core::StrHash tech = solidIdx == 0 ? core::StrHash("wireframe") : core::StrHash("unlit");

                                const auto* pTech = pMaterialManager_->getTechForMaterial(
                                    pMat,
                                    tech,
                                    IPrimativeContext::VERTEX_FMT,
                                    PermatationFlags::Instanced);

                                const auto* pPerm = pTech->pPerm;
                                const auto* pVariableState = pTech->pVariableState;
                                auto variableStateSize = pVariableState->getStateSize();

                                const auto& lodCounts = shapeCnts[solidIdx];
                                for (size_t lodIdx = 0; lodIdx < lodCounts.size(); lodIdx++) {
                                    uint32_t numShapes = lodCounts[lodIdx];
                                    if (numShapes == 0) {
                                        continue;
                                    }

                                    const auto& lodRes = shapeRes.lods[lodIdx];

                                    // dispatch N draw calls for this shapes lod.
                                    // it's more than one draw call if the instance data for a single shapes lod
                                    // spans multiple instance data pages.
                                    while (numShapes > 0) {
                                        if (curPageSpace == 0) {
                                            ++curInstPage;

                                            if (curInstPage >= instPages.size()) {
                                                numShapes = 0;
                                                continue;
                                            }

                                            curPageSpace = maxPerPage;
                                        }

                                        const uint32_t batchSize = core::Min<uint32_t>(curPageSpace, numShapes);
                                        const uint32_t batchOffset = maxPerPage - curPageSpace;

                                        numShapes -= batchSize;
                                        curPageSpace -= batchSize;

                                        // the page we are drawing from.
                                        auto& instPage = instPages[curInstPage];

                                        auto* pDrawInstanced = primBucket.addCommand<render::Commands::DrawInstancedIndexed>(primSortKeyBase + static_cast<uint32_t>(x), variableStateSize);
                                        pDrawInstanced->startInstanceLocation = batchOffset;
                                        pDrawInstanced->instanceCount = batchSize;
                                        pDrawInstanced->startIndexLocation = lodRes.startIndex;
                                        pDrawInstanced->baseVertexLocation = lodRes.baseVertex;
                                        pDrawInstanced->indexCountPerInstance = lodRes.indexCount;
                                        pDrawInstanced->stateHandle = pPerm->stateHandle;
                                        core::zero_object(pDrawInstanced->vertexBuffers);
                                        pDrawInstanced->vertexBuffers[VertexStream::VERT] = shapeRes.vertexBuf;
                                        pDrawInstanced->vertexBuffers[VertexStream::INSTANCE] = instPage.instBufHandle;
                                        pDrawInstanced->indexBuffer = shapeRes.indexbuf;
                                        pDrawInstanced->resourceState = *pVariableState; // slice it in.

                                        if (variableStateSize) {
                                            char* pAuxData = render::CommandPacket::getAuxiliaryMemory(pDrawInstanced);
                                            std::memcpy(pAuxData, pVariableState->getDataStart(), pVariableState->getStateSize());
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                // ~shapeData
            }
        }

        //primBucket.sort();
        primBucket.sort();

        pRender->submitCommandPackets(primBucket);
    }

    for (uint16_t i = 0; i < PrimContext::ENUM_COUNT; i++) {
        if (i == PrimContext::PERSISTENT) {
            if (!clearPersistent_) {
                continue;
            }

            clearPersistent_ = false;
        }

        auto& context = primContexts_[i];
        if (!context.isEmpty()) {
            context.reset();
        }
    }

    // in a really silly place currently.
    if (gEnv->pFontSys && drawVars_.drawFontDebug()) {
        auto* pFont = gEnv->pFontSys->getDefault();
        auto* pContext = getPrimContext(engine::PrimContext::CONSOLE);

        pFont->DrawTestText(pContext, frame.timeInfo);
    }
}

IPrimativeContext* X3DEngine::getPrimContext(PrimContext::Enum user)
{
    return &primContexts_[user];
}

IMaterialManager* X3DEngine::getMaterialManager(void)
{
    return pMaterialManager_;
}

model::IModelManager* X3DEngine::getModelManager(void)
{
    return pModelManager_;
}

anim::IAnimManager* X3DEngine::getAnimManager(void)
{
    return pAnimManager_;
}

fx::IEffectManager* X3DEngine::getEffectManager(void)
{
    return pEffectManager_;
}

IWorld3D* X3DEngine::create3DWorld(physics::IScene* pPhysScene)
{
    auto* pPrimContex = &primContexts_[engine::PrimContext::MISC3D];

    return X_NEW(engine::World3D, g_3dEngineArena, "3DWorld")(drawVars_, pPrimContex, pCBufMan_, pPhysScene, g_3dEngineArena);
}

void X3DEngine::release3DWorld(IWorld3D* pWorld)
{
    X_DELETE(pWorld, g_3dEngineArena);
}

void X3DEngine::addWorldToActiveList(IWorld3D* pWorld)
{
    worlds_.append(pWorld);
}

void X3DEngine::removeWorldFromActiveList(IWorld3D* pWorld)
{
    auto idx = worlds_.find(pWorld);
    if (idx != WorldArr::invalid_index) {
        worlds_.remove(pWorld);
    }
}

// =======================================

void X3DEngine::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
    // do nothing for now.
    X_UNUSED(jobSys);
    X_UNUSED(name);
}

// =======================================

void X3DEngine::Command_ClearPersistent(core::IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    clearPersistent_ = true;
}

X_NAMESPACE_END
