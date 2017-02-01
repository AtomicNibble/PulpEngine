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
#include "Drawing\PrimativeContext.h"

#include <CmdBucket.h>

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
	if (s_var_drawArea_ == -1)
	{
		const XCamera& cam = cam_;
		const Vec3f camPos = cam.getPosition();

		camArea_ = -1;
		outsideWorld_ = !IsPointInAnyArea(camPos, camArea_);


		// if we are outside world draw all.
		// or usePortals is off.
		if (camArea_ < 0 || s_var_usePortals_ == 0)
		{
			// add all areas
			for (const auto& area : areas_)
			{
				SetAreaVisibleAndCull(pJob, area.areaNum);
			}
		}
		else
		{
			// begin culling this area, while we work out what other area's we can potentially see.
			SetAreaVisibleAndCull(pJob, camArea_);

			if (s_var_drawCurrentAreaOnly_ != 1)
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

				auto* pSyncJob = jobSys.CreateEmtpyJob();

				Area::AreaPortalArr::ConstIterator apIt = area.portals.begin();
				for (; apIt != area.portals.end(); ++apIt)
				{
					const AreaPortal& portal = *apIt;

					// make sure this portal is facing away from the view (early out)
					float dis = portal.plane.distance(camPos);
					if (dis < -0.1f) {
						continue;
					}

					// now create a job todo the rest of the flooding for this portal
					PortalFloodJobData jobData = { ps, portal, dis, camPos, camPlanes };
					auto* pFloodJob = jobSys.CreateMemberJobAsChild<Level>(pSyncJob, this, &Level::FloodThroughPortal_job, jobData);
					jobSys.Run(pFloodJob);
				}

				// we must wait for them before leaving this scope.
				jobSys.Run(pSyncJob);
				jobSys.Wait(pSyncJob);
			}
		}
	}
	else if (s_var_drawArea_ < safe_static_cast<int, size_t>(areas_.size()))
	{
		// force draw just this area even if outside world.
		SetAreaVisibleAndCull(pJob, s_var_drawArea_);
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
		FloodViewThroughArea_r(pJob, floodData.camPos, floodData.areaPortal.areaTo, farPlane, &newStack);
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

	FloodViewThroughArea_r(pJob, floodData.camPos, floodData.areaPortal.areaTo, farPlane, &newStack);

}


void Level::FloodViewThroughArea_r(core::V2::Job* pParentJob, const Vec3f origin, int32_t areaNum, 
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
	SetAreaVisibleAndCull(pParentJob, areaNum, ps);

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
			FloodViewThroughArea_r(pParentJob, origin, portal.areaTo, farPlane, &newStack);
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

		FloodViewThroughArea_r(pParentJob, origin, portal.areaTo, farPlane, &newStack);
	}
}

void Level::SetAreaVisibleAndCull(core::V2::Job* pParentJob, int32_t areaNum, const PortalStack* ps)
{
	// this is where we "dispatch" a child job to cull this area
	// if this area is visible from multiple portals we have two culling jobs one with each portal stack.
	// which means we need to store them seperatly.
	// where to store them tho since we can have multiple per a area.
	// make just collect them and pass?

	Area& area = areas_[areaNum];

	{
		core::Spinlock::ScopedLock lock(lock_);
		visibleAreas_.push_back(&area);
	}

	// we need to work out which visible portal index to use.
	// should we just inc the index?
	// or have some logic for working it out :/ ?
	const int32_t visPortalIdx = core::atomic::Increment(&area.cusVisPortalIdx);

	if (ps) {
		area.visPortals[visPortalIdx].planes = ps->portalPlanes;
	}

	AreaCullJobData data = { areaNum, visPortalIdx };
	auto* pJob = pJobSys_->CreateMemberJobAsChild<Level>(pParentJob, this, &Level::CullArea_job, data);

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
				uint32_t entId = modelRefs_.areaRefs[i].entId;
				const level::StaticModel& sm = staticModels_[entId - 1];

				auto type = cam_.cullSphere_ExactT(sm.boundingSphere);
				if (type == CullResult::OVERLAP)
				{
					if (s_var_cullEnts_ > 1)
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
				uint32_t entId = modelRefs_.areaRefs[i].entId;
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
			const size_t total = core::accumulate(pArea->visPortals.begin(), pArea->visPortals.begin() + pArea->cusVisPortalIdx, 0_sz, [](const AreaVisiblePortal& avp) {
				return avp.visibleEnts.size();
			});

			// make engouth space for them all
			pArea->visibleEnts.reserve(total);

			// create a que that gives us the smallets lists first.
			auto cmp = [](const AreaVisiblePortal::EntIdArr* lhs, const AreaVisiblePortal::EntIdArr* rhs) { return lhs->size() < rhs->size(); };
			std::priority_queue<const AreaVisiblePortal::EntIdArr*, core::FixedArray<const AreaVisiblePortal::EntIdArr*, 16>, decltype(cmp)> que(cmp);

			for (int32_t i = 0; i < pArea->cusVisPortalIdx; i++)
			{
				const auto& visEnts = pArea->visPortals[i].visibleEnts;
#if X_DEBUG
				X_ASSERT(std::is_sorted(visEnts.begin(), visEnts.end()), "Not sorted")();
#endif // X_DEBUG
				que.push(&visEnts);
			}

			auto& destArr = pArea->visibleEnts;

			// copy first two into dest then merge.
			auto pVisEnts1 = que.top();
			que.pop();
			std::copy(pVisEnts1->begin(), pVisEnts1->end(), std::back_inserter(destArr));
		
			auto mergepoint = destArr.end();
			auto pVisEnts2 = que.top();
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
			pArea->visibleEnts.swap(pArea->visPortals[0].visibleEnts);
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

	auto* pJobs = jobSys.parallel_for_member_child<Level, Area*>(pJob, del, visibleAreas_.data(), safe_static_cast<uint32_t>(visibleAreas_.size()), core::V2::CountSplitter32(1));
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
		const auto& visEnts = pArea->visibleEnts;

		auto* pJobs = jobSys.parallel_for_member_child<Level>(pJob, del, visEnts.data(), safe_static_cast<uint32_t>(visEnts.size()),
			core::V2::CountSplitter32(16) // will likley need tweaking, props even made a var.
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

		model::XModel* pModel = sm.pModel;


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

		const float distanceFromCam = cam_.getPosition().distance(sm.pos);
		
		if (pModel->HasLods())
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
			pModel->createRenderBuffersForLod(lodIdx);
		}

		const model::MeshHeader& mesh = pModel->getLodMeshHdr(lodIdx);
		const auto& renderMesh = pModel->getLodRenderMesh(lodIdx);

		addMeshTobucket(mesh, renderMesh, distanceFromCam);
	}
}

void Level::addMeshTobucket(const model::MeshHeader& mesh, const model::XRenderMesh& renderMesh, const float distanceFromCam)
{
	render::CommandBucket<uint32_t>* pDepthBucket = nullptr;


	render::VertexBufferHandleArr vertexBuffers = renderMesh.getVBBuffers();

	// we want to remove vertex buffer handles we don't need.
	const core::StrHash tech("unlit");

	// now we render :D !
	for (size_t subIdx = 0; subIdx < mesh.numSubMeshes; subIdx++)
	{
		const model::SubMeshHeader* pSubMesh = mesh.subMeshHeads[subIdx];

		engine::Material* pMat = pSubMesh->pMat;
		engine::MaterialTech* pTech = pMaterialManager_->getTechForMaterial(pMat, tech, render::shader::VertexFormat::P3F_T2F_C4B);

		if (!pTech) {
			continue;
		}

		const auto* pPerm = pTech->pPerm;
		const auto stateHandle = pPerm->stateHandle;
		const auto* pVariableState = pTech->pVariableState;
		auto variableStateSize = pVariableState->getStateSize();


		uint32_t sortKey = static_cast<uint32_t>(distanceFromCam);

		render::Commands::DrawIndexed* pDraw = pDepthBucket->addCommand<render::Commands::DrawIndexed>(sortKey, variableStateSize);
		pDraw->indexCount = pSubMesh->numIndexes;
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
	if (s_var_drawArea_ == -1)
	{
		const XCamera& cam = cam_;
		const Vec3f camPos = cam.getPosition();

		camArea_ = -1;
		outsideWorld_ = !IsPointInAnyArea(camPos, camArea_);


		// if we are outside world draw all.
		// or usePortals is off.
		if (camArea_ < 0 || s_var_usePortals_ == 0)
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

			if (s_var_drawCurrentAreaOnly_ != 1)
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
	else if (s_var_drawArea_ < safe_static_cast<int, size_t>(areas_.size()))
	{
		// force draw just this area even if outside world.
		SetAreaVisible(s_var_drawArea_);
	}
}


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


void Level::DrawPortalDebug(void) const
{
	using namespace render;

	if (s_var_drawPortals_ > 0 && !outsideWorld_)
	{
#if 0
		XAuxGeomRenderFlags flags = AuxGeom_Defaults::Def3DRenderflags;

		// 0=off 1=solid 2=wire 3=solid_dt 4=wire_dt
		flags.SetAlphaBlendMode(AuxGeom_AlphaBlendMode::AlphaBlended);

		if (s_var_drawPortals_ == 2 || s_var_drawPortals_ == 4)
		{
			flags.SetFillMode(AuxGeom_FillMode::FillModeWireframe);
		}

		if (s_var_drawPortals_ == 3 || s_var_drawPortals_ == 4)
		{
			flags.SetDepthWriteFlag(AuxGeom_DepthWrite::DepthWriteOn);
			flags.SetDepthTestFlag(AuxGeom_DepthTest::DepthTestOn);
		}
		else
		{
			flags.SetDepthWriteFlag(AuxGeom_DepthWrite::DepthWriteOff);
			flags.SetDepthTestFlag(AuxGeom_DepthTest::DepthTestOff);
		}

		flags.SetCullMode(AuxGeom_CullMode::CullModeNone);
#endif

	//	pAux_->setRenderFlags(flags);


		// draw the portals.
		AreaArr::ConstIterator areaIt = areas_.begin();
		for (; areaIt != areas_.end(); ++areaIt)
		{
			if (!IsAreaVisible(*areaIt)) {
				continue;
			}

			Area::AreaPortalArr::ConstIterator apIt = areaIt->portals.begin();
			for (; apIt != areaIt->portals.end(); ++apIt)
			{
				const AreaPortal& portal = *apIt;

#if 1
				if (IsAreaVisible(areas_[portal.areaTo]))
				{
					pPrimContex_->drawTriangle(portal.debugVerts.ptr(),
						portal.debugVerts.size(), Colorf(0.f, 1.f, 0.f, 0.35f));
				}
				else
				{
					pPrimContex_->drawTriangle(portal.debugVerts.ptr(),
						portal.debugVerts.size(), Colorf(1.f, 0.f, 0.f, 0.3f));
				}
#else
		

				AABB box;
				portal.pWinding->GetAABB(box);

				if (areas_[portal.areaTo].frameID == frameID_)
				{
					pAux_->drawAABB(
						box, Vec3f::zero(), true, Colorf(0.f, 1.f, 0.f, 0.45f)
						);
				}
				else
				{
					pAux_->drawAABB(
						box, Vec3f::zero(), true, Colorf(1.f, 0.f, 0.f, 1.f)
						);
				}
#endif
			}
		}
	}
}

X_NAMESPACE_END