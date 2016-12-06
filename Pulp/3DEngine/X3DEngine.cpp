#include "stdafx.h"
#include "X3DEngine.h"

#include <ITimer.h>
#include <IMaterial.h>
#include <IRender.h>
#include <IRenderAux.h>
#include <IConsole.h>
#include <IGui.h>

#include "Material\MaterialManager.h"
#include "Model\ModelManager.h"

#include "CmdBucket.h"

#include "Drawing\CBufferManager.h"
#include "Drawing\VariableStateManager.h"

#include <Time\StopWatch.h>
#include <String\StringHash.h>


X_NAMESPACE_BEGIN(engine)

ICore* X3DEngine::pCore_ = nullptr;

core::ITimer* X3DEngine::pTimer_ = nullptr;
core::IFileSys* X3DEngine::pFileSys_ = nullptr;
core::IConsole* X3DEngine::pConsole_ = nullptr;
render::IRender* X3DEngine::pRender_ = nullptr;

// 3d
engine::XMaterialManager* X3DEngine::pMaterialManager_ = nullptr;
model::XModelManager* X3DEngine::pModelManager_ = nullptr;

gui::XGuiManager* X3DEngine::pGuiManger_ = nullptr;

CBufferManager* X3DEngine::pCBufMan_ = nullptr;

VariableStateManager* X3DEngine::pVariableStateMan_ = nullptr;


//------------------------------------------
engine::gui::IGui* pGui = nullptr;


X3DEngine::X3DEngine(core::MemoryArenaBase* arena) :
	primContexts_{ arena, arena, arena }
{

}

X3DEngine::~X3DEngine()
{

}


void X3DEngine::registerVars(void)
{

}

void X3DEngine::registerCmds(void)
{
	ADD_COMMAND_MEMBER("map", this, X3DEngine, &X3DEngine::Command_Map, core::VarFlag::SYSTEM, "Loads a map");
	ADD_COMMAND_MEMBER("devmap", this, X3DEngine, &X3DEngine::Command_DevMap, core::VarFlag::SYSTEM, "Loads a map in developer mode");


}

bool X3DEngine::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pCore);
	X_ASSERT_NOT_NULL(gEnv->pTimer);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);
	X_ASSERT_NOT_NULL(gEnv->pConsole);
	X_ASSERT_NOT_NULL(gEnv->pRender);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);

	pCore_ = gEnv->pCore;
	pTimer_ = gEnv->pTimer;
	pFileSys_ = gEnv->pFileSys;
	pConsole_ = gEnv->pConsole;
	pRender_ = gEnv->pRender;

	// register some file types.
	gEnv->pHotReload->addfileType(this, "level");
	gEnv->pHotReload->addfileType(this, "map");

	pCBufMan_ = X_NEW(CBufferManager, g_3dEngineArena, "CBufMan")(g_3dEngineArena, pRender_);
	pVariableStateMan_ = X_NEW(VariableStateManager, g_3dEngineArena, "StateMan");

	pMaterialManager_ = X_NEW(engine::XMaterialManager, g_3dEngineArena, "MaterialManager")(*pVariableStateMan_);
	if (!pMaterialManager_->Init()) {
		return false;
	}

	pModelManager_ = X_NEW(model::XModelManager, g_3dEngineArena, "ModelManager");
	if (!pModelManager_->Init()) {
		return false;
	}

	pGuiManger_ = &guisMan_;
	if (!guisMan_.Init()) {
		return false;
	}

	// init the prim context states.
	for (auto& primcon : primContexts_)
	{
		if (!primcon.createStates(pRender_)) {
			return false;
		}
	}

	level::Level::Init();

	return true;
}

void X3DEngine::ShutDown(void)
{
	X_LOG0("3DEngine", "Shutting Down");

	gEnv->pHotReload->addfileType(nullptr, "level");
	gEnv->pHotReload->addfileType(nullptr, "map");

	guisMan_.Shutdown();

	for (auto& primcon : primContexts_)
	{
		primcon.freeStates(pRender_);
	}

	if (pModelManager_) {
		pModelManager_->ShutDown();
		X_DELETE(pModelManager_, g_3dEngineArena);
	}

	if (pMaterialManager_) {
		pMaterialManager_->ShutDown();
		X_DELETE(pMaterialManager_, g_3dEngineArena);
	}

	level::Level::ShutDown();
}

void X3DEngine::release(void)
{
	X_DELETE(this,g_3dEngineArena);
}

void X3DEngine::Update(core::FrameData& frame)
{
	X_PROFILE_BEGIN("3DUpdate", core::ProfileSubSys::ENGINE3D);


//	Matrix44f view = Matrix44f::identity();
//	Matrix44f viewProj;
//	MatrixOrthoOffCenterRH(&viewProj, 0, 1680, 1050, 0, -1e10f, 1e10);

	

	pCBufMan_->update(frame);

}

void X3DEngine::OnFrameBegin(void)
{
	X_PROFILE_BEGIN("3DFrameBegin", core::ProfileSubSys::ENGINE3D);

	level_.update();
	if (level_.canRender()) {
		level_.render();
	}

	// draw me some gui baby
	if (pGui) {
	//	pRender_->setGUIShader();
	//	pGui ->Redraw();
	}

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



	// ok so lets dump out the primative context.
	// I will need to have some sort of known time that it's 
	// safe to start creating gpu commands for the context.
	// since te


	// we could populate this command bucket with a job for each primContext.
	// and just shove the index at MSB of key to keep order.
	size_t totalElems = 0;


	for (uint16_t i = 0; i < engine::PrimContext::ENUM_COUNT; i++)
	{
		const auto& context = primContexts_[i];
		if (!context.isEmpty())
		{
			const auto& elems = context.getUnsortedBuffer();

			totalElems += elems.size();
		}
	}

	if (totalElems > 0)
	{

		XCamera cam;
		XViewPort viewPort;

		viewPort.set(1680, 1050);

		render::CmdPacketAllocator cmdBucketAllocator(g_3dEngineArena, totalElems * 256);
		cmdBucketAllocator.createAllocaotrsForThreads(*gEnv->pJobSys);
		render::CommandBucket<uint32_t> primBucket(g_3dEngineArena, cmdBucketAllocator, totalElems + 512, cam, viewPort);


		primBucket.appendRenderTarget(pRender_->getCurBackBuffer());

		// the updating of dirty font buffers should happen regardless of
		// prim drawing.
		// unless only way to draw text is with prim humm.... !
		gEnv->pFontSys->appendDirtyBuffers(primBucket);

#if 1
#if 0
		for (uint16_t i = 0; i < engine::PrimContext::ENUM_COUNT; i++)
		{
			auto& context = primContexts_[i];
			if (!context.isEmpty())
			{
				// create a command(s) to update the VB data.
				context.appendDirtyBuffers(primBucket);

				const auto& elems = context.getUnsortedBuffer();
				auto vertexPageHandles = context.getVertBufHandles();

				for (size_t x = 0; x < elems.size(); x++)
				{
					const auto& elem = elems[x];
					auto* pMat = elem.pMaterial;

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
					const auto* pTech = pMat->getTech(tech, IPrimativeContext::VERTEX_FMT);

					const auto stateHandle = pTech->stateHandle;
					const auto* pVariableState = pTech->pVariableState;
					auto variableStateSize = pVariableState->getStateSize();

					render::Commands::Draw* pDraw = primBucket.addCommand<render::Commands::Draw>(static_cast<uint32_t>(x + 10), variableStateSize);
					pDraw->startVertex = elem.vertexOffs;
					pDraw->vertexCount = elem.numVertices;
					pDraw->stateHandle = stateHandle;
					pDraw->resourceState = *pVariableState; // slice the sizes into command.
					// set the vertex handle to correct one.
					core::zero_object(pDraw->vertexBuffers);
					pDraw->vertexBuffers[VertexStream::VERT] = vertexPageHandles[elem.pageIdx];

					if (variableStateSize)
					{
						// variable state data.
						char* pAuxData = render::CommandPacket::getAuxiliaryMemory(pDraw);
						std::memcpy(pAuxData, pVariableState->getDataStart(), pVariableState->getStateSize());
					}
				}
			}
		}
#endif

#else
		for (uint16_t i = 0; i < engine::PrimContext::ENUM_COUNT; i++)
		{
			auto& context = primContexts_[i];
			if (!context.isEmpty())
			{
				const auto& elems = context.getUnsortedBuffer();
				const auto& verts = context.getVerts();
				const auto vertexBuf = context.getVertBufHandle();


				auto* pUpdateVb = primBucket.addCommand<render::Commands::CopyVertexBufferData>(0, 0);
				pUpdateVb->pData = verts.data();
				pUpdateVb->size = context.getVertBufBytes(); 
				pUpdateVb->vertexBuffer = vertexBuf;


#if 1
#if 1

#if 0
				typedef render::Commands::ResourceState<1, 1, 1> ResState;
				ResState state;
				state.numTextStates = 0;
				state.numVertexBufs = 1;
				state.numCbs = 1;
				state._pad = 0;
				state.vbs[0] = vertexBuf;
				state.cbs[0] = vertexBuf;
				// textures
				state.tex[0].sampler.filter = render::FilterType::LINEAR_MIP_NONE;
				state.tex[0].sampler.repeat = render::TexRepeat::TILE_BOTH;
				state.tex[0].slot = render::TextureSlot::DIFFUSE;
				state.tex[0].textureId = 0;


				for (size_t x = 0; x < elems.size(); x++)
				{
					const auto& elem = elems[x];
					const auto flags = elem.flags;

					const auto textureId = flags.getTextureId();

					render::Commands::Draw* pDraw = primBucket.addCommand<render::Commands::Draw>(static_cast<uint32_t>(x + 10), 0);
					pDraw->startVertex = elem.vertexOffs;
					pDraw->vertexCount = elem.numVertices;
					pDraw->stateHandle = elem.stateHandle; 
					// copy the state sizes.
					pDraw->resourceState = state;

					// state data.
					char* pAuxData = render::CommandPacket::getAuxiliaryMemory(pDraw);
					std::memcpy(pAuxData, state.getData(), ResState::STATE_DATA_SIZE);
				}

#endif

#else
				struct JobData
				{
					const PrimativeContext::PushBufferEntry* pElem;
					render::CommandBucket<uint32_t>* pPrimBucket;
					render::VertexBufferHandle vertexBuf;
					render::ConstantBufferHandle constBuf;
					uint32_t hash;
				};

				auto* pRootJob = gEnv->pJobSys->CreateJob(core::V2::JobSystem::EmptyJob, nullptr);
	

				for (size_t x = 0; x < elems.size(); x++)
				{
					JobData data;
					data.pElem = &elems[x];
					data.pPrimBucket = &primBucket;
					data.vertexBuf = vertexBuf;
					data.hash = static_cast<uint32_t>(x) + 10;

					auto* pJob = gEnv->pJobSys->CreateJobAsChild(pRootJob, [](core::V2::JobSystem&, size_t, core::V2::Job*, void* pData_) {
						
						const JobData* pData = reinterpret_cast<const JobData*>(pData_);

						const auto& elem = *pData->pElem;
						const auto flags = elem.flags;
						const auto textureId = flags.getTextureId();


						typedef render::Commands::ResourceState<0, 1, 1> ResState;
						ResState state;
						state.numTextStates = 0;
						state.numVertexBufs = 1;
						state.numCbs = 1;
						state._pad = 0;
						state.vbs[0] = pData->vertexBuf;
						state.cbs[0] = pData->vertexBuf;


						render::Commands::Draw* pDraw = pData->pPrimBucket->addCommand<render::Commands::Draw>(pData->hash, ResState::STATE_DATA_SIZE);
						pDraw->startVertex = elem.vertexOffs;
						pDraw->vertexCount = elem.numVertices;
						pDraw->stateHandle = elem.stateHandle;
						pDraw->resourceState = state;

						char* pAuxData = render::CommandPacket::getAuxiliaryMemory(pDraw);
						std::memcpy(pAuxData, state.getData(), ResState::STATE_DATA_SIZE);

						//core::zero_object(pDraw->vertexBuffers);
						//pDraw->vertexBuffers[VertexStream::VERT] = pData->vertexBuf;

					}, data);

					gEnv->pJobSys->Run(pJob);
				}

				gEnv->pJobSys->Run(pRootJob);
				gEnv->pJobSys->Wait(pRootJob);
#endif


#else

				const auto& front = elems.front();
				uint32_t curFlags = front.flags;

				render::Commands::Draw* pDraw = primBucket.addCommand<render::Commands::Draw>(curFlags, 0);
				pDraw->startVertex = front.vertexOffs;
				pDraw->vertexCount = front.numVertices;
				core::zero_object(pDraw->vertexBuffers);
				pDraw->vertexBuffers[VertexStream::VERT] = vertexBuf;
				pDraw->stateHandle = stateHandle; // we need state :cry:

				for (size_t x = 1; x < elems.size(); x++)
				{
					const auto& elem = elems[x];
					uint32_t flags = elem.flags;

					// while the flags don't change, we append. WHY?
					if (flags == curFlags)
					{
						render::Commands::Draw* pChild = primBucket.appendCommand<render::Commands::Draw>(pDraw, 0);
						pChild->startVertex = elem.vertexOffs;
						pChild->vertexCount = elem.numVertices;
						core::zero_object(pDraw->vertexBuffers);
						pChild->vertexBuffers[VertexStream::VERT] = vertexBuf;
						pDraw->stateHandle = stateHandle;
					}
					else
					{
						pDraw = primBucket.addCommand<render::Commands::Draw>(flags, 0);
						pDraw->startVertex = elem.vertexOffs;
						pDraw->vertexCount = elem.numVertices;
						core::zero_object(pDraw->vertexBuffers);
						pDraw->vertexBuffers[VertexStream::VERT] = vertexBuf;
						pDraw->stateHandle = stateHandle;

						// update flags.
						curFlags = flags;
					}
				}
#endif
			}
		}

#endif

		primBucket.sort();

		pRender_->submitCommandPackets(primBucket);

	}

	for (uint16_t i = 0; i < engine::PrimContext::ENUM_COUNT; i++)
	{
		auto& context = primContexts_[i];
		if (!context.isEmpty())
		{
			context.reset();
		}
	}
}


IPrimativeContext* X3DEngine::getPrimContext(PrimContext::Enum user)
{
	return &primContexts_[user];
}

void X3DEngine::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
	// do nothing for now.
	X_UNUSED(jobSys);
	X_UNUSED(name);

}

// =======================================



void X3DEngine::LoadMap(const char* pMapName)
{
	X_ASSERT_NOT_NULL(pMapName);

	level_.Load(pMapName);
}

void X3DEngine::LoadDevMap(const char* pMapName)
{
	X_ASSERT_NOT_NULL(pMapName);

	// this should not really duplicate anything.
	// set some vars then just load the map normaly tbh.

	LoadMap(pMapName);
}



// Commands

void X3DEngine::Command_Map(core::IConsoleCmdArgs* Cmd)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->p3DEngine);

	if (Cmd->GetArgCount() != 2)
	{
		X_WARNING("3DEngine", "map <mapname>");
		return;
	}

	const char* pMapName = Cmd->GetArg(1);

	LoadMap(pMapName);
}

void X3DEngine::Command_DevMap(core::IConsoleCmdArgs* Cmd)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->p3DEngine);

	if (Cmd->GetArgCount() != 2)
	{
		X_WARNING("3DEngine", "devmap <mapname>");
		return;
	}

	const char* pMapName = Cmd->GetArg(1);

	LoadDevMap(pMapName);
}

// ~Commands

X_NAMESPACE_END
