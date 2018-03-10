#include "stdafx.h"
#include "Level.h"

#include <IFileSys.h>
#include <IRender.h>
#include <IRenderMesh.h>
#include <IRenderCommands.h>
#include <ITimer.h>

#include <Threading\JobSystem2.h>
#include <Memory\MemCursor.h>
#include <Math\XWinding.h>

#include "Material\MaterialManager.h"
#include "Drawing\PrimativeContext.h"
#include "Drawing\CBufferManager.h"

#include <CmdBucket.h>
#include <CBuffer.h>

#include <queue>


X_NAMESPACE_BEGIN(level)


namespace
{
	struct PortalFloodJobData
	{
		PortalStack& basePortalStack;
		const AreaPortal& areaPortal;
		float dis;
		Vec3f camPos;
		int32_t areaFrom;
		const Planef (&camPlanes)[FrustumPlane::ENUM_COUNT];
	};

	struct AreaCullJobData
	{
		int32_t areaIdx;
		int32_t visPortalIdx;
	};

} // namespaec

void Level::FindVisibleArea_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	visibleAreas_.clear();

	// clear all active portals from last frame.
	clearVisPortals();

	// we want to find out what area we are in.
	// and then dispatch jobs to recursivly find the others.
	if (vars_.drawArea() == -1)
	{
		const XCamera& cam = cam_;
		const Vec3f camPos = cam.getPosition();

		camArea_ = -1;
		outsideWorld_ = !IsPointInAnyArea(camPos, camArea_);


		// if we are outside world draw all.
		// or usePortals is off.
		if (camArea_ < 0 || vars_.usePortals() == 0)
		{
			// add all areas
			for (const auto& area : areas_)
			{
				SetAreaVisibleAndCull(pJob, area.areaNum, -1);
			}
		}
		else
		{
			// begin culling this area, while we work out what other area's we can potentially see.
			SetAreaVisibleAndCull(pJob, camArea_, -1);

			if (vars_.drawCurrentAreaOnly() != 1)
			{
				// any portals in this area?
				if (!AreaHasPortals(camArea_)) {
					// should we exit the game, cus this is likley a box map :|
					return;
				}

				// build up the cams planes.
				const Planef camPlanes[FrustumPlane::ENUM_COUNT] = {
					cam.getFrustumPlane(FrustumPlane::LEFT),
					cam.getFrustumPlane(FrustumPlane::RIGHT),
					cam.getFrustumPlane(FrustumPlane::TOP),
					cam.getFrustumPlane(FrustumPlane::BOTTOM),
					cam.getFrustumPlane(FrustumPlane::FAR),
					cam.getFrustumPlane(FrustumPlane::NEAR)
				};

				const Planef farPlane = cam.getFrustumPlane(FrustumPlane::FAR);

				PortalStack	ps;

				for (size_t i = 0; i < FrustumPlane::ENUM_COUNT; i++) {
					ps.portalPlanes.append(camPlanes[i]);
				}

				// work out what other areas we can see from this one.
				const Area& area = areas_[camArea_];

				auto* pSyncJob = jobSys.CreateEmtpyJob(JOB_SYS_SUB_ARG_SINGLE(core::profiler::SubSys::ENGINE3D));

				Area::AreaPortalArr::ConstIterator apIt = area.portals.begin();
				for (; apIt != area.portals.end(); ++apIt)
				{
					const AreaPortal& portal = *apIt;

					// make sure this portal is facing away from the view (early out)
					float dis = portal.plane.distance(camPos);
					if (dis < -0.0f) {
						continue;
					}

					// now create a job todo the rest of the flooding for this portal
					PortalFloodJobData jobData = { ps, portal, dis, camPos, camArea_, camPlanes };
					auto* pFloodJob = jobSys.CreateMemberJobAsChild<Level>(pSyncJob, this, &Level::FloodThroughPortal_job, jobData JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
					jobSys.Run(pFloodJob);
				}

				// we must wait for them before leaving this scope.
				jobSys.Run(pSyncJob);
				jobSys.Wait(pSyncJob);
			}
		}
	}
	else if (vars_.drawArea() < safe_static_cast<int, size_t>(areas_.size()))
	{
		// force draw just this area even if outside world.
		SetAreaVisibleAndCull(pJob, vars_.drawArea(), -1);
	}

}

void Level::FloodThroughPortal_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);

	const PortalFloodJobData& floodData = *reinterpret_cast<PortalFloodJobData*>(pData);
	// we want to work out if this portal is visible.
	// then travell through it and find all the others.
	const Planef& farPlane = floodData.camPlanes[4];
	
	if (floodData.dis < 1.0f)
	{
		// it's visible.
		PortalStack	newStack;
		newStack = floodData.basePortalStack;
		newStack.pPortal = &floodData.areaPortal;
		newStack.pNext = &floodData.basePortalStack;
		FloodViewThroughArea_r(pJob, floodData.camPos, floodData.areaPortal.areaTo, floodData.areaFrom, farPlane, &newStack);
		return;
	}
	
	XWinding w;

	// work out if the portal is visible, by clipping the portals winding with that of the cam planes.
	w = (*floodData.areaPortal.pWinding);

	for (uint32_t j = 0; j < FrustumPlane::ENUM_COUNT; j++) {
		if (!w.clipInPlace(floodData.camPlanes[j], 0)) {
			break;
		}
	}
	if (!w.getNumPoints()) {
		return;	// portal not visible
	}

	// go through this portal
	PortalStack	newStack;
	newStack.pPortal = &floodData.areaPortal;
	newStack.pNext = &floodData.basePortalStack;

	Vec3f v1, v2;

	size_t addPlanes = w.getNumPoints();
	if (addPlanes > PortalStack::MAX_PORTAL_PLANES) {
		addPlanes = PortalStack::MAX_PORTAL_PLANES;
	}

	for (size_t i  = 0; i < addPlanes; i++)
	{
		size_t j = i + 1;
		if (j == w.getNumPoints()) {
			j = 0;
		}

		v1 = floodData.camPos - w[i].asVec3();
		v2 = floodData.camPos - w[j].asVec3();

		Vec3f normal = v2.cross(v1);
		normal.normalize();

		// if it is degenerate, skip the plane
		if (normal.length() < 0.01f) {
			continue;
		}

		Planef& plane = newStack.portalPlanes.AddOne();
		plane.setNormal(normal);
		plane.setDistance(normal * floodData.camPos);
		plane = -plane;
	}

	// far plane
	newStack.portalPlanes.append(farPlane);

	// the last stack plane is the portal plane
	newStack.portalPlanes.append(-floodData.areaPortal.plane);

	FloodViewThroughArea_r(pJob, floodData.camPos, floodData.areaPortal.areaTo, floodData.areaFrom, farPlane, &newStack);

}


void Level::FloodViewThroughArea_r(core::V2::Job* pParentJob, const Vec3f origin, int32_t areaNum, int32_t areaFrom,
	const Planef& farPlane, const PortalStack* ps)
{
	X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t, size_t>(NumAreas())),
		"areaNum out of range")(areaNum, NumAreas());

	PortalStack	newStack;
	const PortalStack* check;
	XWinding w;
	float dis;
	size_t j;

	const Area& area = areas_[areaNum];

	// add this area.
	SetAreaVisibleAndCull(pParentJob, areaNum, areaFrom, ps);

	// see what portals we can see.

	Area::AreaPortalArr::ConstIterator apIt = area.portals.begin();
	for (; apIt != area.portals.end(); ++apIt)
	{
		const AreaPortal& portal = *apIt;

		// make sure this portal is facing away from the view
		dis = portal.plane.distance(origin);
		if (dis < -0.0f) {
			continue;
		}

		// make sure the portal isn't in our stack trace,
		// which would cause an infinite loop
		for (check = ps; check; check = check->pNext) {
			if (check->pPortal == &portal) {
				break;	// don't recursively enter a stack
			}
		}
		if (check) {
			continue;	// already in stack
		}

		// if we are very close to the portal surface, don't bother clipping
		// it, which tends to give epsilon problems that make the area vanish
		if (dis < 1.0f)
		{
			// go through this portal
			newStack = *ps;
			newStack.pPortal = &portal;
			newStack.pNext = ps;
			FloodViewThroughArea_r(pParentJob, origin, portal.areaTo, areaNum, farPlane, &newStack);
			continue;
		}

		// clip the portal winding to all of the planes
		w = *portal.pWinding;

		for (j = 0; j < ps->portalPlanes.size(); j++)
		{
			if (!w.clipInPlace(ps->portalPlanes[j], 0))
			{
				break;
			}
		}
		if (!w.getNumPoints()) {
			continue;	// portal not visible
		}

		// go through this portal
		newStack.pPortal = &portal;
		newStack.pNext = ps;
		newStack.portalPlanes.clear();

		size_t i, addPlanes;
		Vec3f	v1, v2;

		addPlanes = w.getNumPoints();
		if (addPlanes > PortalStack::MAX_PORTAL_PLANES) {
			addPlanes = PortalStack::MAX_PORTAL_PLANES;
		}

		for (i = 0; i < addPlanes; i++)
		{
			j = i + 1;
			if (j == w.getNumPoints()) {
				j = 0;
			}

			v1 = origin - w[i].asVec3();
			v2 = origin - w[j].asVec3();

			Vec3f normal = v2.cross(v1);
			normal.normalize();

			// if it is degenerate, skip the plane
			if (normal.length() < 0.01f) {
				continue;
			}

			Planef& plane = newStack.portalPlanes.AddOne();
			plane.setNormal(normal);
			plane.setDistance(normal * origin);
			plane = -plane;
		}

		newStack.portalPlanes.append(farPlane);

		// the last stack plane is the portal plane
		newStack.portalPlanes.append(-portal.plane);

		FloodViewThroughArea_r(pParentJob, origin, portal.areaTo, areaNum, farPlane, &newStack);
	}
}

void Level::SetAreaVisibleAndCull(core::V2::Job* pParentJob, int32_t areaNum, int32_t areaFrom, const PortalStack* ps)
{
	// this is where we "dispatch" a child job to cull this area
	// if this area is visible from multiple portals we have two culling jobs one with each portal stack.
	// which means we need to store them seperatly.
	// where to store them tho since we can have multiple per a area.
	// make just collect them and pass?


	// when we check if a plane is behind us we allow some laps.
	// so this means we can go throught a portal then end up coming back throught it in the other
	// direction. we need to handle this correctly as it will result in potentially too many visible portals for a area.
	if (areaNum == camArea_ && areaFrom >= 0) {
		X_WARNING("Level", "Portal flood ended back in cam area skipping visibility for stack");
		return;
	}

	Area& area = areas_[areaNum];

	{
		core::Spinlock::ScopedLock lock(visAreaLock_);

		if (std::find(visibleAreas_.begin(), visibleAreas_.end(), &area) == visibleAreas_.end())
		{
			visibleAreas_.push_back(&area);
		}
	}

	// we need to work out which visible portal index to use.
	// should we just inc the index?
	// or have some logic for working it out :/ ?
	int32_t visPortalIdx = -1;

	if (area.maxVisPortals)
	{
		visPortalIdx = core::atomic::Increment(&area.cusVisPortalIdx);

		X_ASSERT(visPortalIdx < area.maxVisPortals, "Area entered from more portals than expected")(areaNum, areaFrom, visPortalIdx, area.maxVisPortals, ps);

		area.visPortals[visPortalIdx].areaFrom = areaFrom;

		if (ps) {
			// this is thread safe as each thread gets a diffrent idx.
			area.visPortals[visPortalIdx].planes = ps->portalPlanes;
		}
	}

	AreaCullJobData data = { areaNum, visPortalIdx };
	auto* pJob = pJobSys_->CreateMemberJobAsChild<Level>(pParentJob, this, &Level::CullArea_job, data JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));

	pJobSys_->Run(pJob);
}


void Level::CullArea_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);

	const AreaCullJobData& jobData = *reinterpret_cast<AreaCullJobData*>(pData);

	Area& area = areas_[jobData.areaIdx];

	const FileAreaRefHdr& areaModelsHdr = modelRefs_.areaRefHdrs[jobData.areaIdx];
	size_t i = areaModelsHdr.startIndex;
	size_t end = i + areaModelsHdr.num;


	if (jobData.visPortalIdx < 0)
	{
		return;
	}

	auto& visPortal = area.visPortals[jobData.visPortalIdx];

	visPortal.visibleEnts.clear();

	// we want to cull everything in this area.
	// it's either culled with the camera or the portal planes.
	if (visPortal.planes.isEmpty())
	{
		// process all the static models in this area
		{
			for (; i < end; i++)
			{
				uint32_t entId = modelRefs_.areaRefs[i].modelId;
				const level::StaticModel& sm = staticModels_[entId - 1];

				auto type = cam_.cullSphere_ExactT(sm.boundingSphere);
				if (type == CullResult::OVERLAP)
				{
					if (vars_.cullEnts() > 1)
					{
						// sphere test says overlap.
						// see if we can cull it with AABB.
						type = cam_.cullAABB_FastT(sm.boundingBox);
						if (type == CullResult::EXCLUSION)
						{
							// got culled by AABB
							continue;
						}
					}
				}
				else if (type == CullResult::EXCLUSION)
				{
					frameStats_.culledModels++;
					continue;
				}

				// this model is visible.
				visPortal.visibleEnts.emplace_back(entId);
			}
		}
	}
	else
	{
		// get the portal planes we are checking against.
		const auto& planes = visPortal.planes;

		// process all the static models in this area
		{
			for (; i < end; i++)
			{
				uint32_t entId = modelRefs_.areaRefs[i].modelId;
				const level::StaticModel& sm = staticModels_[entId - 1];

				// sphere check
				const Vec3f& center = sm.boundingSphere.center();
				const float radius = -sm.boundingSphere.radius();

				for (const auto& plane : planes)
				{
					const float d = plane.distance(center);
					if (d < radius) {
						continue; // not visible in this stack, might be in another.
					}
				}

				// AABB check?
				// ...


				// if we get here we are visible.
				visPortal.visibleEnts.emplace_back(entId);
			}
		}
	}

	// sort them.
	std::sort(visPortal.visibleEnts.begin(), visPortal.visibleEnts.end());

	// we need to do stuff..

}



void Level::BuildVisibleAreaFlags_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	for (auto* pArea : visibleAreas_)
	{
		SetAreaVisible(pArea->areaNum);
	}
}


void Level::MergeVisibilityArrs_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	// ok so all the visible area's have been found and models have been culled.
	// we just need to do some post process on the cull reuslts.
	// since we have seperate cull lists for each portal that enters a area.
	// 

	for (auto* pArea : visibleAreas_)
	{	
		if (pArea->cusVisPortalIdx > 0)
		{
			const size_t total = core::accumulate(pArea->visPortals.begin(), pArea->visPortals.begin() + pArea->cusVisPortalIdx + 1, 0_sz, [](const AreaVisiblePortal& avp) {
				return avp.visibleEnts.size();
			});

			// make engouth space for them all
			pArea->areaVisibleEnts.reserve(total);
			pArea->areaVisibleEnts.clear();

			// create a que that gives us the smallets lists first.
			auto cmp = [](const AreaVisiblePortal::EntIdArr* lhs, const AreaVisiblePortal::EntIdArr* rhs) { return lhs->size() < rhs->size(); };
			std::priority_queue<const AreaVisiblePortal::EntIdArr*, core::FixedArray<const AreaVisiblePortal::EntIdArr*, 16>, decltype(cmp)> que(cmp);

			for (int32_t i = 0; i < pArea->cusVisPortalIdx + 1; i++)
			{
				const auto& visEnts = pArea->visPortals[i].visibleEnts;
#if X_DEBUG
				X_ASSERT(std::is_sorted(visEnts.begin(), visEnts.end()), "Not sorted")();
#endif // X_DEBUG
				que.push(&visEnts);
			}

			X_ASSERT(que.size() >= 2, "source code error should be atleast 2 in que")(que.size());

			auto& destArr = pArea->areaVisibleEnts;

			// copy first two into dest then merge.
			auto* pVisEnts1 = que.top();
			que.pop();
			std::copy(pVisEnts1->begin(), pVisEnts1->end(), std::back_inserter(destArr));
		
			auto mergepoint = destArr.end();
			auto* pVisEnts2 = que.top();
			que.pop();
			std::copy(pVisEnts2->begin(), pVisEnts2->end(), std::back_inserter(destArr));

			std::inplace_merge(destArr.begin(), mergepoint, destArr.end());

			// remove dupes
			auto pte = std::unique(destArr.begin(), destArr.end());
			destArr.erase(pte, destArr.end());

			// now we need to just keep copying and merging the others.
			while (!que.empty())
			{
				auto pVisEnts = que.top();
				que.pop();

				mergepoint = destArr.end();
				std::copy(pVisEnts->begin(), pVisEnts->end(), std::back_inserter(destArr));
				std::inplace_merge(destArr.begin(), mergepoint, destArr.end());

				// remove dupes
				pte = std::unique(destArr.begin(), destArr.end());
				destArr.erase(pte, destArr.end());
			}

			std::vector<int> foat;
			foat.erase(foat.begin(), foat.end());

		}
		else
		{
			pArea->areaVisibleEnts.swap(pArea->visPortals[0].visibleEnts);
		}
	}
}

void Level::DrawVisibleAreaGeo_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	// here we add draw calls for the visible area's to cmdBucket.
	// 
	core::Delegate<void(Area**, uint32_t)> del;
	del.Bind<Level, &Level::DrawAreaGeo>(this);

	auto* pJobs = jobSys.parallel_for_member_child<Level, Area*>(pJob, del, visibleAreas_.data(), 
		safe_static_cast<uint32_t>(visibleAreas_.size()), core::V2::CountSplitter32(1) JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));

	jobSys.Run(pJobs);
}

void Level::DrawVisibleStaticModels_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	// we just dispatch parallel jobs for each area.
	// this will then create multiple jobs for each area
	// depending on how many ents are in the area

	core::Delegate<void(const uint32_t*, uint32_t)> del;
	del.Bind<Level, &Level::DrawStaticModels>(this);

	for (auto* pArea : visibleAreas_)
	{
		const auto& visEnts = pArea->areaVisibleEnts;

		auto* pJobs = jobSys.parallel_for_member_child<Level>(pJob, del, visEnts.data(), safe_static_cast<uint32_t>(visEnts.size()),
			core::V2::CountSplitter32(16) // will likley need tweaking, props even made a var.
			JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D)
		);

		jobSys.Run(pJobs);
	}
}

void Level::DrawAreaGeo(Area** pAreas, uint32_t num)
{
	for (uint32_t i = 0; i < num; i++)
	{
		Area* pArea = pAreas[i];


		const model::MeshHeader& mesh = *pArea->pMesh;
		const auto& renderMesh = pArea->renderMesh;
		const float distanceFromCam = 0; // humm

		addMeshTobucket(mesh, renderMesh, distanceFromCam);

	}
}

void Level::DrawStaticModels(const uint32_t* pEntIds, uint32_t num)
{

	for (uint32_t i = 0; i < num; i++)
	{
		const uint32_t entId = pEntIds[i];
		level::StaticModel& sm = staticModels_[entId - 1];

		// should this be allowed?
		if (!sm.pModel) {
			continue;
		}

		model::RenderModel* pModel = static_cast<model::RenderModel*>(sm.pModel);

		// is this a good place todo the bucketing?
		// like placing the model in multiple buckets
		// seams like a good palce since we are fully parralel here.
		// should we also handle lod logic here?
		// how to handle instancing?
		// since if we have 10 models that are the same we want to draw them as a batch.
		// for static models we can work out models that are the same so for a visible area we have the info about what can be instanced.
		// i think per area batching should work well enougth in practice tho.
		// then that would need processing / culling a little diffrent so that i end up with list of ents for instanced drawing
		// which can then have buffers made with all the instanced info and populate each frame and draw.
		// i don't think pre making the instanced buffers per a area is worth it as it will make culling harder.
		// so for now we just handle the rendering of a model.
		size_t lodIdx = 0;

		const float distanceFromCam = cam_.getPosition().distance(sm.transform.pos);
		
		if (pModel->hasLods())
		{
			// work out which lod to select based on distance.
			lodIdx = pModel->lodIdxForDistance(distanceFromCam);

			// now just because we want to render with this lod don't mean we can
			// for example we may have not uploaded the mesh for this lod to the gpu
			
			if (!pModel->canRenderLod(lodIdx))
			{
				// lets try pick another lod.
				// higer or lower?
				while (lodIdx > 0 && !pModel->canRenderLod(lodIdx))
				{
					--lodIdx;
				}
			}
		}

		// upload if needed.
		if (!pModel->canRenderLod(lodIdx))
		{
			pModel->createRenderBuffersForLod(lodIdx, gEnv->pRender);
		}

		const model::MeshHeader& mesh = pModel->getLodMeshHdr(lodIdx);
		const auto& renderMesh = pModel->getLodRenderMesh(lodIdx);

		addMeshTobucket(mesh, renderMesh, distanceFromCam);
	}
}

void Level::addMeshTobucket(const model::MeshHeader& mesh, const model::XRenderMesh& renderMesh, const float distanceFromCam)
{
	render::CommandBucket<uint32_t>* pDepthBucket = pBucket_;


	render::VertexBufferHandleArr vertexBuffers = renderMesh.getVBBuffers();

	// we want to remove vertex buffer handles we don't need.
	// but can we do that on a perm mesh bases?
	// since with have diffrent materials 
	// so it's the requirement of the material as to what buffers it needs.
	const core::StrHash tech("unlit");

	// now we render :D !
	for (size_t subIdx = 0; subIdx < mesh.numSubMeshes; subIdx++)
	{
		const model::SubMeshHeader* pSubMesh = mesh.subMeshHeads[subIdx];

		engine::Material* pMat = pSubMesh->pMat;
		engine::MaterialTech* pTech = engine::gEngEnv.pMaterialMan_->getTechForMaterial(pMat, tech, render::shader::VertexFormat::P3F_T4F_C4B_N3F, 
			engine::PermatationFlags::VertStreams);

		if (!pTech) {
			continue;
		}


		const auto* pPerm = pTech->pPerm;
		const auto stateHandle = pPerm->stateHandle;
		const auto* pVariableState = pTech->pVariableState;
		auto variableStateSize = pVariableState->getStateSize();


		uint32_t sortKey = static_cast<uint32_t>(distanceFromCam);

		render::Commands::CopyConstantBufferData* pCBufUpdate = nullptr;
		if (variableStateSize)
		{
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
							pCBufUpdate = pDepthBucket->addCommand<render::Commands::CopyConstantBufferData>(static_cast<uint32_t>(sortKey),cb.getBindSize());
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

		render::Commands::DrawIndexed* pDraw = nullptr;

		if (pCBufUpdate) 
		{
			pDraw = pDepthBucket->appendCommand<render::Commands::DrawIndexed>(pCBufUpdate, variableStateSize);
		}
		else
		{
			pDraw = pDepthBucket->addCommand<render::Commands::DrawIndexed>(sortKey, variableStateSize);
		}

		pDraw->indexCount = pSubMesh->numFaces * 3;
		pDraw->startIndex = pSubMesh->startIndex;
		pDraw->baseVertex = pSubMesh->startVertex;
		pDraw->stateHandle = stateHandle;
		pDraw->resourceState = *pVariableState; // slice the sizes into command.
												// set the vertex handle to correct one.
		pDraw->indexBuffer = renderMesh.getIBBuffer();
		pDraw->vertexBuffers = vertexBuffers;

		if (variableStateSize)
		{
			// variable state data.
			char* pAuxData = render::CommandPacket::getAuxiliaryMemory(pDraw);
			std::memcpy(pAuxData, pVariableState->getDataStart(), pVariableState->getStateSize());
		}
	}
}


// -------------------------------------------------------------------------



void Level::clearVisPortals(void)
{
	for (auto& a : areas_)
	{
		a.cusVisPortalIdx = -1;

		for (auto& vp : a.visPortals)
		{
			vp.visibleEnts.clear();
			vp.planes.clear();
		}
	}
}


void Level::FloodVisibleAreas(void)
{
	if (vars_.drawArea() == -1)
	{
		const XCamera& cam = cam_;
		const Vec3f camPos = cam.getPosition();

		camArea_ = -1;
		outsideWorld_ = !IsPointInAnyArea(camPos, camArea_);


		// if we are outside world draw all.
		// or usePortals is off.
		if (camArea_ < 0 || vars_.usePortals() == 0)
		{
			// add all areas
			AreaArr::ConstIterator it = areas_.begin();
			for (; it != areas_.end(); ++it)
			{
				SetAreaVisible(it->areaNum);
			}
		}
		else
		{
			SetAreaVisible(camArea_);

			if (vars_.drawCurrentAreaOnly() != 1)
			{
				if (!AreaHasPortals(camArea_)) {
					return;
				}

				XWinding w;

				// build up the cams planes.
				const Planef camPlanes[FrustumPlane::ENUM_COUNT] = {
					cam.getFrustumPlane(FrustumPlane::LEFT),
					cam.getFrustumPlane(FrustumPlane::RIGHT),
					cam.getFrustumPlane(FrustumPlane::TOP),
					cam.getFrustumPlane(FrustumPlane::BOTTOM),
					cam.getFrustumPlane(FrustumPlane::FAR),
					cam.getFrustumPlane(FrustumPlane::NEAR)
				};

				const Planef farPlane = cam.getFrustumPlane(FrustumPlane::FAR);

				PortalStack	ps;
				size_t i, j;

				for (i = 0; i < FrustumPlane::ENUM_COUNT; i++) {
					ps.portalPlanes.append(camPlanes[i]);
				}


				// work out what other areas we can see from this one.
				const Area& area = areas_[camArea_];

				Area::AreaPortalArr::ConstIterator apIt = area.portals.begin();
				for (; apIt != area.portals.end(); ++apIt)
				{
					const AreaPortal& portal = *apIt;

					// make sure this portal is facing away from the view
					float dis = portal.plane.distance(camPos);
					if (dis < -0.1f) {
						continue;
					}


					if (dis < 1.0f) {
						// go through this portal
						PortalStack	newStack;
						newStack = ps;
						newStack.pPortal = &portal;
						newStack.pNext = &ps;
						FloodViewThroughArea_r(camPos, portal.areaTo, farPlane, &newStack);
						continue;
					}

					// work out if the portal is visible, by clipping the portals winding with that of the cam planes.
					w = (*portal.pWinding);

					for (j = 0; j < FrustumPlane::ENUM_COUNT; j++) {
						if (!w.clipInPlace(camPlanes[j], 0)) {
							break;
						}
					}
					if (!w.getNumPoints()) {
						continue;	// portal not visible
					}

					// go through this portal
					PortalStack	newStack;
					newStack.pPortal = &portal;
					newStack.pNext = &ps;

					Vec3f v1, v2;

					size_t addPlanes = w.getNumPoints();
					if (addPlanes > PortalStack::MAX_PORTAL_PLANES) {
						addPlanes = PortalStack::MAX_PORTAL_PLANES;
					}

					for (i = 0; i < addPlanes; i++)
					{
						j = i + 1;
						if (j == w.getNumPoints()) {
							j = 0;
						}

						v1 = camPos - w[i].asVec3();
						v2 = camPos - w[j].asVec3();

						Vec3f normal = v2.cross(v1);
						normal.normalize();

						// if it is degenerate, skip the plane
						if (normal.length() < 0.01f) {
							continue;
						}

						Planef& plane = newStack.portalPlanes.AddOne();
						plane.setNormal(normal);
						plane.setDistance(normal * camPos);
						plane = -plane;
					}

					// far plane
					newStack.portalPlanes.append(farPlane); 

					// the last stack plane is the portal plane
					newStack.portalPlanes.append(-portal.plane);

					FloodViewThroughArea_r(camPos, portal.areaTo, farPlane, &newStack);

				}
			}
		}
	}
	else if (vars_.drawArea() < safe_static_cast<int, size_t>(areas_.size()))
	{
		// force draw just this area even if outside world.
		SetAreaVisible(vars_.drawArea());
	}
}

#if 0
void Level::DrawVisibleAreas(void)
{
	AreaArr::ConstIterator it = areas_.begin();

	for (; it != areas_.end(); ++it)
	{
		const Area& area = *it;

		if (IsAreaVisible(area)) {
			DrawArea(area);
		}
	}
}
#endif

// ==================================================================

size_t Level::NumPortalsInArea(int32_t areaNum) const
{
	X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t, size_t>(NumAreas())),
		"areaNum out of range")(areaNum, NumAreas());

	return areas_[areaNum].portals.size();
}

bool Level::AreaHasPortals(int32_t areaNum) const
{
	return NumPortalsInArea(areaNum) > 0;
}

#if 0
void Level::FlowViewThroughPortals(const int32_t areaNum, const Vec3f origin,
	size_t numPlanes, const Planef* pPlanes)
{
	PortalStack	ps;
	size_t	i;

	// copy the planes.
	for (i = 0; i < numPlanes; i++) {
		ps.portalPlanes.append(pPlanes[i]);
	}

	if (areaNum < 0)
	{
		// outside draw everything.
		for (i = 0; i < areas_.size(); i++) {
			SetAreaVisible(safe_static_cast<int32_t>(i), &ps);
		}
	}
	else
	{
		const XCamera& cam = cam_;
		const Planef farPlane = cam.getFrustumPlane(FrustumPlane::FAR);

		FloodViewThroughArea_r(origin, areaNum, farPlane, &ps);
	}
}
#endif

void Level::FloodViewThroughArea_r(const Vec3f origin, int32_t areaNum, const Planef& farPlane,
	const PortalStack* ps)
{
	X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t, size_t>(NumAreas())),
		"areaNum out of range")(areaNum, NumAreas());

	PortalStack	newStack;
	const PortalStack* check;
	XWinding w;
	float dis;
	size_t j;

	const Area& area = areas_[areaNum];

	// add this area.
//	SetAreaVisible(areaNum, ps);

	// see what portals we can see.

	Area::AreaPortalArr::ConstIterator apIt = area.portals.begin();
	for (; apIt != area.portals.end(); ++apIt)
	{
		const AreaPortal& portal = *apIt;

		// make sure this portal is facing away from the view
		dis = portal.plane.distance(origin);
		if (dis < -0.1f) {
			continue;
		}

		// make sure the portal isn't in our stack trace,
		// which would cause an infinite loop
		for (check = ps; check; check = check->pNext) {
			if (check->pPortal == &portal) {
				break;	// don't recursively enter a stack
			}
		}
		if (check) {
			continue;	// already in stack
		}

		// if we are very close to the portal surface, don't bother clipping
		// it, which tends to give epsilon problems that make the area vanish
		if (dis < 1.0f)
		{
			// go through this portal
			newStack = *ps;
			newStack.pPortal = &portal;
			newStack.pNext = ps;
			FloodViewThroughArea_r(origin, portal.areaTo, farPlane, &newStack);
			continue;
		}

		// clip the portal winding to all of the planes
		w = *portal.pWinding;

		for (j = 0; j < ps->portalPlanes.size(); j++)
		{
			if (!w.clipInPlace(ps->portalPlanes[j], 0))
			{
				break;
			}
		}
		if (!w.getNumPoints()) {
			continue;	// portal not visible
		}

		// go through this portal
		newStack.pPortal = &portal;
		newStack.pNext = ps;
		newStack.portalPlanes.clear();

		size_t i, addPlanes;
		Vec3f	v1, v2;

		addPlanes = w.getNumPoints();
		if (addPlanes > PortalStack::MAX_PORTAL_PLANES) {
			addPlanes = PortalStack::MAX_PORTAL_PLANES;
		}

		for (i = 0; i < addPlanes; i++)
		{
			j = i + 1;
			if (j == w.getNumPoints()) {
				j = 0;
			}

			v1 = origin - w[i].asVec3();
			v2 = origin - w[j].asVec3();

			Vec3f normal = v2.cross(v1);
			normal.normalize();

			// if it is degenerate, skip the plane
			if (normal.length() < 0.01f) {
				continue;
			}

			Planef& plane = newStack.portalPlanes.AddOne();
			plane.setNormal(normal);
			plane.setDistance(normal * origin);
			plane = -plane;
		}

		newStack.portalPlanes.append(farPlane);

		// the last stack plane is the portal plane
		newStack.portalPlanes.append(-portal.plane);

		FloodViewThroughArea_r(origin, portal.areaTo, farPlane, &newStack);
	}
}



X_NAMESPACE_END