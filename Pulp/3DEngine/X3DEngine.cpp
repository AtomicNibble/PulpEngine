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
#include "Gui\MenuManger.h"

#include "CmdBucket.h"

#include "Drawing\CBufferManager.h"
#include "Drawing\VariableStateManager.h"

#include "World\World3D.h"


X_NAMESPACE_BEGIN(engine)

using namespace core::string_view_literals;

EngineEnv gEngEnv;

//------------------------------------------

X3DEngine::X3DEngine(core::MemoryArenaBase* arena) :
    pMaterialManager_(nullptr),
    pTextureManager_(nullptr),
    pModelManager_(nullptr),
    pAnimManager_(nullptr),
    pEffectManager_(nullptr),
    pMenuManager_(nullptr),
    pCBufMan_(nullptr),
    pVariableStateMan_(nullptr),
    primContexts_{
        {primResources_, PrimativeContext::Mode::Mode3D, PrimativeContext::MaterialSet::BASE, arena}, // sound
        {primResources_, PrimativeContext::Mode::Mode3D, PrimativeContext::MaterialSet::BASE, arena}, // physics
        {primResources_, PrimativeContext::Mode::Mode3D, PrimativeContext::MaterialSet::BASE, arena}, // misc3d
        {primResources_, PrimativeContext::Mode::Mode3D, PrimativeContext::MaterialSet::BASE, arena}, // persistent
        {primResources_, PrimativeContext::Mode::Mode2D, PrimativeContext::MaterialSet::BASE_2D, arena}, // gui
        {primResources_, PrimativeContext::Mode::Mode2D, PrimativeContext::MaterialSet::BASE_2D, arena}, // profile
        {primResources_, PrimativeContext::Mode::Mode2D, PrimativeContext::MaterialSet::BASE_2D, arena}  // console
    },
    clearPersistent_(false),
    coreEventReg_(false),
    worlds_(arena)
{
    pixelBufffers_.fill(nullptr);

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
    ADD_COMMAND_MEMBER("rClearPersistent", this, X3DEngine, &X3DEngine::Command_ClearPersistent,
        core::VarFlag::SYSTEM, "Clears persistent primatives");

    ADD_COMMAND_MEMBER("rWriteBufferToFile", this, X3DEngine, &X3DEngine::Command_WriteBufferToFile,
        core::VarFlag::SYSTEM, "Write a frame buffer to file <2d,3d>");
}

bool X3DEngine::init(void)
{
    X_PROFILE_NO_HISTORY_BEGIN("3DEngineInit", core::profiler::SubSys::ENGINE3D);
    X_ASSERT(gEngEnv.p3DEngine_ == nullptr, "Doble init")();


    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pCore);
    X_ASSERT_NOT_NULL(gEnv->pTimer);
    X_ASSERT_NOT_NULL(gEnv->pFileSys);
    X_ASSERT_NOT_NULL(gEnv->pConsole);
    X_ASSERT_NOT_NULL(gEnv->pRender);
    X_ASSERT_NOT_NULL(gEnv->pPhysics);

    coreEventReg_ = gEnv->pCore->GetCoreEventDispatcher()->RegisterListener(this);

    auto* pRender = gEnv->pRender;

    pCBufMan_ = X_NEW(CBufferManager, g_3dEngineArena, "CBufMan")(g_3dEngineArena, pRender);
    pVariableStateMan_ = X_NEW(VariableStateManager, g_3dEngineArena, "StateMan");
    pTextureManager_ = X_NEW(engine::TextureManager, g_3dEngineArena, "TextureManager")(g_3dEngineArena);
    pMaterialManager_ = X_NEW(engine::XMaterialManager, g_3dEngineArena, "MaterialManager")(g_3dEngineArena, *pVariableStateMan_, *pCBufMan_);
    pModelManager_ = X_NEW(model::XModelManager, g_3dEngineArena, "ModelManager")(g_3dEngineArena, g_3dEngineArena);
    pAnimManager_ = X_NEW(anim::AnimManager, g_3dEngineArena, "AnimManager")(g_3dEngineArena, g_3dEngineArena);
    pEffectManager_ = X_NEW(fx::EffectManager, g_3dEngineArena, "EffectManager")(g_3dEngineArena, g_3dEngineArena);
    pMenuManager_ = X_NEW(gui::XMenuManager, g_3dEngineArena, "GuiManager")(g_3dEngineArena, pMaterialManager_);

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
    pMenuManager_->registerCmds();
    pMenuManager_->registerVars();

    gEngEnv.pMenuMan_ = pMenuManager_;
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
        if (!pMenuManager_->init()) {
            return false;
        }
    }

    if (!primResources_.init(pRender, pMaterialManager_)) {
        X_ERROR("3DEngine", "Failed to init prim resources");
        return false;
    }

    // so i will need some textures to render into.
    auto dispalyRes = pRender->getDisplayRes();

    if (dispalyRes.x <= 0 || dispalyRes.y <= 0) {
        X_ERROR("3DEngine", "Display res is zero");
        return false;
    }

    auto clearCol2d = drawVars_.clearCol2D();
    auto clearCol3d = drawVars_.clearCol3D();

    // these must be same size.
    pixelBufffers_[PixelBuf::DEPTH_STENCIL] = pRender->createDepthBuffer("$depth_buffer"_sv, dispalyRes);
    pixelBufffers_[PixelBuf::COL_3D] = pRender->createColorBuffer("$rt_3d"_sv, dispalyRes, 1, texture::Texturefmt::R8G8B8A8, clearCol3d);
    
    // this can be any size, but typically have it match display.
    pixelBufffers_[PixelBuf::COL_2D] = pRender->createColorBuffer("$rt_2d"_sv, dispalyRes, 1, texture::Texturefmt::R8G8B8A8, clearCol2d);

#if 0 // some stuff for prelinking video texture to material tech.
    auto* pMat = pMaterialManager_->loadMaterial("video/doom_trailer");
    auto* pTech = pMaterialManager_->getTechForMaterial(pMat, core::StrHash("unlit"),
        render::shader::VertexFormat::P3F_T4F_C4B_N3F, engine::PermatationFlags::VertStreams);

    if (!pTech) {
        X_ERROR("Font", "Failed to get 'unlit' tech for font");
        return false;
    }

    auto* pVid = gEnv->pVideoSys->loadVideo("test/doom_trailer_720");

    pVid->createRenderTexture(pRender);

    auto id = pVid->getTextureID();

    engine::RegisterCtx con;
    con.regs[engine::Register::CodeTexture0] = id;

    if (!pMaterialManager_->setRegisters(pTech, con)) {
        X_ERROR("Font", "Failed to bind fontcache taxture to font state");
        return false;
    }
#endif


    return true;
}

void X3DEngine::shutDown(void)
{
    X_LOG0("3DEngine", "Shutting Down");

    if (coreEventReg_) {
        gEnv->pCore->GetCoreEventDispatcher()->RemoveListener(this);
    }

    auto* pRender = gEnv->pRender;

    for (auto* pPixelBuffer : pixelBufffers_) {
        if (pPixelBuffer) {
            pRender->releasePixelBuffer(pPixelBuffer);
        }
    }

    for (auto& primcon : primContexts_) {
        primcon.freePages(pRender);
    }

    primResources_.releaseResources(pRender, pMaterialManager_);

    if (pMenuManager_) {
        pMenuManager_->shutdown();
        X_DELETE(pMenuManager_, g_3dEngineArena);

        gEngEnv.pMenuMan_ = nullptr;
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

void X3DEngine::clearPersistent(void)
{
    clearPersistent_ = true;
}

void X3DEngine::update(core::FrameData& frame)
{
    X_PROFILE_BEGIN("3DUpdate", core::profiler::SubSys::ENGINE3D);

    pTextureManager_->scheduleStreaming();


    auto dispalyRes = gEnv->pRender->getDisplayRes();
    frame.view.displayRes = dispalyRes;
    frame.view.renderRes = dispalyRes;

    if (gEnv->pVideoSys) {
        gEnv->pVideoSys->unlockBuffers();
    }
}

void X3DEngine::onFrameBegin(core::FrameData& frame)
{
    X_PROFILE_BEGIN("3DFrameBegin", core::profiler::SubSys::ENGINE3D);

    // draw the emitters into prim context.
    for (auto* pWorld : worlds_) {
        static_cast<World3D*>(pWorld)->renderEmitters(frame, &primContexts_[PrimContext::MISC3D]);
    }

    auto* pRender = gEnv->pRender;

    {
        XViewPort& viewPort = frame.view.viewport;
        viewPort.setZ(0.f, 1.f);
        viewPort.set(pixelBufffers_[PixelBuf::COL_3D]->getDimensions());

        render::CmdPacketAllocator cmdBucketAllocator(g_3dEngineArena, 8096 * 12);
        cmdBucketAllocator.createAllocaotrsForThreads(*gEnv->pJobSys);
        render::CommandBucket<uint32_t> geoBucket(g_3dEngineArena, cmdBucketAllocator, 8096 * 5, viewPort);

        geoBucket.appendRenderTarget(pixelBufffers_[PixelBuf::COL_3D], render::RenderTargetFlag::CLEAR);
        geoBucket.setDepthStencil(pixelBufffers_[PixelBuf::DEPTH_STENCIL], render::DepthBindFlag::CLEAR | render::DepthBindFlag::WRITE);

        // clear some buffers.
        // 2d - is cleared as we might not render to it, yet still blend it in (could skip composite)
        {
            auto* pClear = geoBucket.addCommand<render::Commands::ClearColor>(0, 0);
            pClear->pColorBuffer = pRender->getCurBackBuffer();

            pClear = geoBucket.addCommand<render::Commands::ClearColor>(0, 0);
            pClear->pColorBuffer = pixelBufffers_[PixelBuf::COL_2D];
        }

        // TODO: move
        gEnv->pFontSys->appendDirtyBuffers(geoBucket);
        gEnv->pVideoSys->appendDirtyBuffers(geoBucket);

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
                ctx.regs[Register::CodeTexture0] = pixelBufffers_[PixelBuf::DEPTH_STENCIL]->getTexID();

                pMaterialManager_->initStateFromRegisters(pTech, &pDraw->resourceState, ctx);
            }
        }

        geoBucket.sort();

        auto list = pRender->createCommandLists(geoBucket);
        pRender->submitCommandLists(core::make_span(&list, 1));
    }

    {
        PrimativeContextArr prims;

        getPrimsWithData(prims, IPrimativeContext::Mode::Mode3D);
        if (!prims.isEmpty()) 
        {
            size_t totalElems = getTotalElems(prims);

            XViewPort viewPort;
            viewPort.setZ(0.f, 1.f);
            viewPort.set(pixelBufffers_[PixelBuf::COL_3D]->getDimensions());

            render::CmdPacketAllocator cmdBucketAllocator(g_3dEngineArena, totalElems * 1024);
            cmdBucketAllocator.createAllocaotrsForThreads(*gEnv->pJobSys);
            render::CommandBucket<uint32_t> primBucket(g_3dEngineArena, cmdBucketAllocator, totalElems * 4, viewPort);

            primBucket.appendRenderTarget(pixelBufffers_[PixelBuf::COL_3D]);
            primBucket.setDepthStencil(pixelBufffers_[PixelBuf::DEPTH_STENCIL], render::DepthBindFlag::WRITE);

            addPrimsToBucket(frame, primBucket, IPrimativeContext::Mode::Mode3D, core::make_span(prims.begin(), prims.end()));

            // sort + draw
            primBucket.sort();

            auto list = pRender->createCommandLists(primBucket);
            pRender->submitCommandLists(core::make_span(&list, 1));
        }
    }

    // draw 2d!
    renderPrimContex2D(frame, IPrimativeContext::Mode::Mode2D);

    // draw the textures int oframe buffer.
    // sometimes i might want to just build a bucket in single threaded mode.
#if 1
    {
        XViewPort viewPort;
        viewPort.setZ(0.f, 1.f);
        viewPort.set(pRender->getDisplayRes());

        render::CmdPacketAllocator cmdBucketAllocator(g_3dEngineArena, 2048);
        cmdBucketAllocator.createAllocaotrsForThreads(*gEnv->pJobSys);
        render::CommandBucket<uint32_t> bucket(g_3dEngineArena, cmdBucketAllocator, 16, viewPort);

        bucket.appendRenderTarget(pRender->getCurBackBuffer());

        // so i think i'm just gonna draw a quad, that way i can control filtering better.
        TechDefPerm* pTechBlend = pMaterialManager_->getCodeTech(
            core::string("fullscreen"),
            core::StrHash("blend"),
            render::shader::VertexFormat::NONE,
            PermatationFlags::Textured);

        TechDefPerm* pPreMul = pMaterialManager_->getCodeTech(
            core::string("fullscreen"),
            core::StrHash("premul"),
            render::shader::VertexFormat::NONE,
            PermatationFlags::Textured);

       
        X_UNUSED(pTechBlend, pPreMul);

        if (drawVars_.drawBuffer3D())
        {
            auto* pDraw = bucket.addCommand<render::Commands::Draw>(0, pTechBlend->variableStateSize);
            pDraw->startVertex = 0;
            pDraw->vertexCount = 3;
            pDraw->stateHandle = pTechBlend->stateHandle;
            core::zero_object(pDraw->vertexBuffers);
            core::zero_object(pDraw->resourceState);

            if (pTechBlend->variableStateSize) {
                RegisterCtx ctx;
                ctx.regs[Register::CodeTexture0] = pixelBufffers_[PixelBuf::COL_3D]->getTexID();

                pMaterialManager_->initStateFromRegisters(pTechBlend, &pDraw->resourceState, ctx);
            }
        }

        if(drawVars_.drawBuffer2D())
        {
            auto* pDraw = bucket.addCommand<render::Commands::Draw>(1, pTechBlend->variableStateSize);
            pDraw->startVertex = 0;
            pDraw->vertexCount = 3;
            pDraw->stateHandle = pPreMul->stateHandle;
            core::zero_object(pDraw->vertexBuffers);
            core::zero_object(pDraw->resourceState);

            if (pPreMul->variableStateSize) {
                RegisterCtx ctx;
                ctx.regs[Register::CodeTexture0] = pixelBufffers_[PixelBuf::COL_2D]->getTexID();

                pMaterialManager_->initStateFromRegisters(pPreMul, &pDraw->resourceState, ctx);
            }
        }

        // i want to get the pixels for the buffer.

        bucket.sort();

        auto list = pRender->createCommandLists(bucket);
        pRender->submitCommandLists(core::make_span(&list, 1));
    }
#endif


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

void X3DEngine::renderPrimContex2D(core::FrameData& frame, IPrimativeContext::Mode mode)
{
    PrimativeContextArr prims;

    getPrimsWithData(prims, mode);

#if 0
    prims.clear();
    prims.append(&primContexts_[PrimContext::CONSOLE]);

    if (primContexts_[PrimContext::CONSOLE].isEmpty()) {
        return;
    }
#endif

    if (prims.isEmpty()) {
        return;
    }


    size_t totalElems = getTotalElems(prims);

    auto* pRender = gEnv->pRender;

    XViewPort& viewPort = frame.view.viewport;
    viewPort.setZ(0.f, 1.f);
    viewPort.set(pixelBufffers_[PixelBuf::COL_2D]->getDimensions());

    // invisible?
    if (viewPort.getWidth() <= 0 || viewPort.getHeight() <= 0) {
        X_WARNING("Engine", "Skipping render viewport is empty rect");
        return;
    }

    render::CmdPacketAllocator cmdBucketAllocator(g_3dEngineArena, totalElems * 1024);
    cmdBucketAllocator.createAllocaotrsForThreads(*gEnv->pJobSys);
    render::CommandBucket<uint32_t> primBucket(g_3dEngineArena, cmdBucketAllocator, totalElems * 4, viewPort);

    primBucket.appendRenderTarget(pixelBufffers_[PixelBuf::COL_2D]);
    primBucket.setDepthStencil(pixelBufffers_[PixelBuf::DEPTH_STENCIL]);

    addPrimsToBucket(frame, primBucket, mode, core::make_span(prims.begin(), prims.end()));

    // sort + draw
    primBucket.sort();

    auto list = pRender->createCommandLists(primBucket);
    pRender->submitCommandLists(core::make_span(&list, 1));
}

void X3DEngine::addPrimsToBucket(core::FrameData& frame, render::CommandBucket<uint32_t>& primBucket,
    IPrimativeContext::Mode mode, core::span<PrimativeContext*> prims)
{
    const auto is2D = mode == IPrimativeContext::Mode::Mode2D;

    auto* pRender = gEnv->pRender;

    for (int32_t i = 0; i < prims.size(); i++)
    {
        auto& context = *prims[i];
        X_ASSERT(!context.isEmpty(), "Contex is empty")();

        // this is so the cb update is between each prim, but don't think this is needed.
        // if we only drawing orth. TODO:
        const uint32_t primSortKeyBase = static_cast<uint32_t>(i) * 16384_ui32;

        if (pCBufMan_->update(frame, is2D)) {
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

            const core::StrHash tech("unlit");

            const auto* pTech = pMaterialManager_->getTechForMaterial(pMat, tech, IPrimativeContext::VERTEX_FMT);

            const auto* pPerm = pTech->pPerm;
            const auto stateHandle = pPerm->stateHandle;
            const auto* pVariableState = pTech->pVariableState;
            auto variableStateSize = pVariableState->getStateSize();


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
            Material* pMat = primResources_.getMaterial(PrimativeContext::MaterialSet::BASE, render::TopoType::TRIANGLELIST);

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

void X3DEngine::getPrimsWithData(PrimativeContextArr& prims, IPrimativeContext::Mode mode)
{
    for (uint32_t i = 0; i < engine::PrimContext::ENUM_COUNT; i++) {
        auto& context = primContexts_[i];
        if (context.isEmpty()) {
            continue;
        }

        if (context.getMode() != mode) {
            continue;
        }

        prims.push_back(&context);
    }
}

size_t X3DEngine::getTotalElems(const PrimativeContextArr& prims)
{
    size_t totalElems = core::accumulate(prims.begin(), prims.end(), 0_sz, [](PrimativeContext* pPrim) {
        const auto& elems = pPrim->getUnsortedBuffer();
        const auto& objBufs = pPrim->getShapeArrayBuffers();

        auto size = elems.size();

        for (auto& objBuf : objBufs) {
            size += objBuf.size();
        }
        return size;
    });

    return totalElems;
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

gui::IMenuManager* X3DEngine::getMenuManager(void)
{
    return pMenuManager_;
}

IWorld3D* X3DEngine::create3DWorld(physics::IScene* pPhysScene)
{
    auto* pPrimContex = &primContexts_[engine::PrimContext::MISC3D];

    return X_NEW(engine::World3D, g_3dEngineArena, "3DWorld")(drawVars_, pPrimContex, pCBufMan_, pPhysScene, g_3dEngineArena);
}

void X3DEngine::release3DWorld(IWorld3D* pWorld)
{
    X_ASSERT(worlds_.find(pWorld) == WorldArr::invalid_index, "World is still ative")(pWorld);
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

void X3DEngine::OnCoreEvent(const CoreEventData& ed)
{
    if (ed.event == CoreEvent::RENDER_RES_CHANGED)
    {
        // TODO: when doing stuff like rendering scene at 50% might want to change out buffers.
        Vec2i newDim(ed.renderRes.width, ed.renderRes.height);

        auto* pRender = gEnv->pRender;

        auto clearCol2d = drawVars_.clearCol2D();
        auto clearCol3d = drawVars_.clearCol3D();

        if (pixelBufffers_[PixelBuf::COL_2D])
        {
            auto dim = pixelBufffers_[PixelBuf::COL_2D]->getDimensions();

            if (dim != newDim)
            {
                pRender->releasePixelBuffer(pixelBufffers_[PixelBuf::COL_2D]);

                pixelBufffers_[PixelBuf::COL_2D] = pRender->createColorBuffer("$rt_2d"_sv,
                    newDim, 1, texture::Texturefmt::A8R8G8B8, clearCol2d);
            }
        }

        if (pixelBufffers_[PixelBuf::COL_3D])
        {
            auto dim = pixelBufffers_[PixelBuf::COL_3D]->getDimensions();

            if (dim != newDim)
            {
                pRender->releasePixelBuffer(pixelBufffers_[PixelBuf::COL_3D]);

                pixelBufffers_[PixelBuf::COL_3D] = pRender->createColorBuffer("$rt_3d"_sv, 
                    newDim, 1, texture::Texturefmt::R8G8B8A8, clearCol3d);
            }
        }

        if (pixelBufffers_[PixelBuf::DEPTH_STENCIL])
        {
            auto dim = pixelBufffers_[PixelBuf::DEPTH_STENCIL]->getDimensions();

            if (dim != newDim)
            {
                pRender->releasePixelBuffer(pixelBufffers_[PixelBuf::DEPTH_STENCIL]);

                pixelBufffers_[PixelBuf::DEPTH_STENCIL] = pRender->createDepthBuffer("$depth_buffer"_sv, newDim);
            }
        }
    }
}

void X3DEngine::Command_ClearPersistent(core::IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    clearPersistent_ = true;
}

void X3DEngine::Command_WriteBufferToFile(core::IConsoleCmdArgs* pCmd)
{
    if (pCmd->GetArgCount() < 2) {
        return;
    }

    auto bufferStr = pCmd->GetArg(1);

    render::IPixelBuffer* pBuffer = nullptr;

    if (core::strUtil::IsEqualCaseInsen(bufferStr, "2d"_sv)) {
        pBuffer = pixelBufffers_[PixelBuf::COL_2D];
    } 
    else if (core::strUtil::IsEqualCaseInsen(bufferStr, "3d"_sv)) {
        pBuffer = pixelBufffers_[PixelBuf::COL_3D];
    }

    if (!pBuffer) {
        X_ERROR("3DEngine", "Unkown buffer: %.*s", bufferStr.length(), bufferStr.data());
        return;
    }

    // need to build a texture file.
    texture::XTextureFile texure(g_3dEngineArena);

    if (!gEnv->pRender->getBufferData(pBuffer, texure)) {
        X_ERROR("3DEngine", "Failed to get frame buffer data");
        return;
    }

    core::Path<> name;
    name.setFmt("framebuffer_%.*s.dds", bufferStr.length(), bufferStr.data());

    core::XFileScoped file;
    if (!file.openFile(name, core::FileFlags::WRITE | core::FileFlags::RECREATE)) {
        X_ERROR("3DEngine", "Failed to open file for writing frame buffer");
        return;
    }

    texture::DDS::XTexLoaderDDS dds;
    if (!dds.saveTexture(file.GetFile(), texure, g_3dEngineArena)) {
        X_ERROR("3DEngine", "Failed to save frame buffer image");
        return;
    }

    X_LOG0("3DEngine", "Saved frame buffer to file...");
}

X_NAMESPACE_END
