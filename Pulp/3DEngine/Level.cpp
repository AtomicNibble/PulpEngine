#include "stdafx.h"
#include "Level.h"

#include <IFileSys.h>
#include <IRender.h>
#include <IRenderMesh.h>
#include <ITimer.h>
#include <IConsole.h>

#include <Memory\MemCursor.h>

#include <Math\XWinding.h>

#include <IRenderAux.h>

X_NAMESPACE_BEGIN(level)

AsyncLoadData::~AsyncLoadData()
{
}

// --------------------------------

AreaNode::AreaNode()
{
	children[0] = -1;
	children[1] = -1;
	commonChildrenArea = -1;
}


AreaPortal::AreaPortal()
{
	areaTo = -1;
	pWinding = nullptr;
}

AreaPortal::~AreaPortal()
{
	if (pWinding) {
		X_DELETE(pWinding, g_3dEngineArena);
	}
}

Area::Area() : portals(g_3dEngineArena)
{
	areaNum = -1;
	pMesh = nullptr;
	pRenderMesh = nullptr;
}

Area::~Area()
{
	pMesh = nullptr; // Area() don't own the memory.

	if (pRenderMesh) {
		pRenderMesh->release();
	}
}

// --------------------------------

int Level::s_var_drawAreaBounds_ = 0;
int Level::s_var_drawPortals_ = 0;
int Level::s_var_drawArea_ = -1;

// --------------------------------

Level::Level() :
areas_(g_3dEngineArena),
areaNodes_(g_3dEngineArena),
stringTable_(g_3dEngineArena)
{
	canRender_ = false;

	pFileData_ = nullptr;
	pAsyncLoadData_ = nullptr;

	pTimer_ = nullptr;
	pFileSys_ = nullptr;

	core::zero_object(fileHdr_);

	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pTimer);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);

	pTimer_ = gEnv->pTimer;
	pFileSys_ = gEnv->pFileSys;
}

Level::~Level()
{
	free();
}

bool Level::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pConsole);

	ADD_CVAR_REF("lvl_drawAreaBounds", s_var_drawAreaBounds_, 0, 0, 1, core::VarFlag::SYSTEM,
		"Draws bounding box around each level area");

	ADD_CVAR_REF("lvl_drawPortals", s_var_drawPortals_, 0, 0, 4, core::VarFlag::SYSTEM,
		"Draws the inter area portals. 0=off 1=solid 2=wire 3=solid_dt 4=wire_dt");

	ADD_CVAR_REF("lvl_drawArea", s_var_drawArea_, -1, -1, level::MAP_MAX_AREAS, core::VarFlag::SYSTEM,
		"Draws the selected area index. -1 = disable");

	

	return true;
}

void Level::ShutDown(void)
{


}

void Level::update(void)
{
	// are we trying to read?
	if (pAsyncLoadData_ && pAsyncLoadData_->waitingForIo_)
	{
		uint32_t numbytes = 0;

		if (pAsyncLoadData_->AsyncOp_.hasFinished(&numbytes))
		{
			pAsyncLoadData_->waitingForIo_ = false;

			if (!pAsyncLoadData_->headerLoaded_)
			{
				pAsyncLoadData_->headerLoaded_ = true;
				ProcessHeader(numbytes);
			}
			else
			{
				// the data is loaded.
				ProcessData(numbytes);
			}
		}
	}
}

void Level::free(void)
{
	canRender_ = false;

#if 0
	// clear render mesh.
	core::Array<AreaModel>::ConstIterator it = areaModels_.begin();
	for (; it != areaModels_.end(); ++it)
	{
		it->pRenderMesh->release();
	}

	areaModels_.free();
#endif

	areas_.free();
	areaNodes_.free();

	if (pFileData_) {
		X_DELETE_ARRAY(pFileData_, g_3dEngineArena);
		pFileData_ = nullptr;
	}

	if (pAsyncLoadData_) 
	{
		if (pAsyncLoadData_->waitingForIo_)
		{
			// cancel the read request
			pAsyncLoadData_->AsyncOp_.cancel();
			// close file.
			gEnv->pFileSys->closeFileAsync(pAsyncLoadData_->pFile_);
		}

		X_DELETE_AND_NULL(pAsyncLoadData_, g_3dEngineArena);
	}
}

bool Level::canRender(void)
{
	return canRender_;
}

bool Level::render(void)
{
	if (!canRender())
		return false;

	AreaArr::ConstIterator it = areas_.begin();

	if (s_var_drawArea_ == -1)
	{
		for (; it != areas_.end(); ++it)
		{
			it->pRenderMesh->render();
		}
	}
	else if (s_var_drawArea_ < safe_static_cast<int, size_t>(areas_.size()))
	{
		areas_[s_var_drawArea_].pRenderMesh->render();
	}


#if 0
	core::Array<AreaModel>::ConstIterator it = areaModels_.begin();

	if (s_var_drawArea_ == -1)
	{
		for (; it != areaModels_.end(); ++it)
		{
			it->pRenderMesh->render();
		}
	}
	else if (s_var_drawArea_ < safe_static_cast<int,size_t>(areaModels_.size()))
	{
		areaModels_[s_var_drawArea_].pRenderMesh->render();
	}

	if (s_var_drawAreaBounds_)
	{
		render::IRenderAux* pAux = gEnv->pRender->GetIRenderAuxGeo();

		pAux->setRenderFlags(render::AuxGeom_Defaults::Def3DRenderflags);

		it = areaModels_.begin();
		for (; it != areaModels_.end(); ++it)
		{
			Vec3f pos = Vec3f::zero();
			pAux->drawAABB(it->pMesh->boundingBox,pos, false, Col_Red);
		}
	}

	if (s_var_drawPortals_ > 0)
	{
		using namespace render;

		IRenderAux* pAux = gEnv->pRender->GetIRenderAuxGeo();
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

		pAux->setRenderFlags(flags);

		// draw portals.
		for (const auto& p : portals_)
		{
			AABB box;
			p.pWinding->GetAABB(box);

			pAux->drawAABB(
				box, Vec3f::zero(), true, Colorf(0.f,0.f,1.f,0.5f)
			);
		}
	}
#endif

	return true;
}


bool Level::Load(const char* mapName)
{
	// does the other level file exsist?
	path_.set(mapName);
	path_.setExtension(level::LVL_FILE_EXTENSION);

	if (!gEnv->pFileSys->fileExists(path_.c_str())) {
		X_ERROR("Level", "could not find level file: \"%s\"",
			path_.c_str());
		return false;
	}

	// free it :)
	free();

	X_LOG0("Level", "Loading level: %s", mapName);
	loadStats_ = LoadStats();
	loadStats_.startTime = pTimer_->GetTimeReal();

	// clear it.
	core::zero_object(fileHdr_);

	core::fileModeFlags mode = core::fileMode::READ;
	core::XFileAsync* pFile = pFileSys_->openFileAsync(path_.c_str(), mode);
	if (!pFile) {
		return false;
	}

	core::XFileAsyncOperation HeaderOp = pFile->readAsync(&fileHdr_, sizeof(fileHdr_), 0);

	pAsyncLoadData_ = X_NEW(AsyncLoadData, g_3dEngineArena, "LevelLoadHandles")(pFile, HeaderOp);
	pAsyncLoadData_->waitingForIo_ = true;
	return true;
}

bool Level::ProcessHeader(uint32_t bytesRead)
{
	if (bytesRead != sizeof(fileHdr_)) {
		X_ERROR("Level", "failed to read header file: %s", path_.fileName());

		return false;
	}

	// is this header valid m'lady?
	if (!fileHdr_.isValid())
	{
		// this header is not valid :Z
		if (fileHdr_.fourCC == LVL_FOURCC_INVALID)
		{
			X_ERROR("Level", "%s file is corrupt, please re-compile.", path_.fileName());
			return false;
		}
	
		X_ERROR("Level", "%s is not a valid level", path_.fileName());
		return false;
	}

	if (fileHdr_.version != LVL_VERSION)
	{
		X_ERROR("Level", "%s has a invalid version. provided: %i required: %i",
			path_.fileName(), fileHdr_.version, LVL_VERSION);
		return false;
	}

	if (fileHdr_.totalDataSize <= 0)
	{
		X_ERROR("Level", "level file is empty");
		return false;
	}

	// require atleast one area.
	if (fileHdr_.numAreas < 1)
	{
		X_ERROR("Level", "Level file has no areas");
		return false;
	}

	// allocate the file data.
	pFileData_ = X_NEW_ARRAY(uint8_t, fileHdr_.totalDataSize, g_3dEngineArena, "LevelBuffer");
	
	pAsyncLoadData_->AsyncOp_ = pAsyncLoadData_->pFile_->readAsync(pFileData_, 
		fileHdr_.totalDataSize, sizeof(fileHdr_));

	pAsyncLoadData_->waitingForIo_ = true;
	return true;
}




bool Level::ProcessData(uint32_t bytesRead)
{
	if (bytesRead != fileHdr_.totalDataSize) {
		X_ERROR("Level", "failed to read level file. read: %i of %i",
			bytesRead, fileHdr_.totalDataSize);
		return false;
	}

	// read string table.
	{
		core::XFileBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::STRING_TABLE);

		if (!stringTable_.SLoad(&file))
		{
			X_ERROR("Level", "Failed to load string table.");
			return false;
		}

		if (!file.isEof())
		{
			X_ERROR("Level", "Failed to fully parse sting table.");
			return false;
		}
	}

	areas_.resize(fileHdr_.numAreas);

	// area data.
	{
		core::StackString<64> meshName;
		core::MemCursor<uint8_t> cursor(pFileData_ + fileHdr_.nodes[FileNodes::AREAS].offset,
			fileHdr_.nodes[FileNodes::AREAS].size);
		uint32_t x, numSub;

		for (int32_t i = 0; i < fileHdr_.numAreas; i++)
		{
			model::MeshHeader* pMesh = cursor.getSeekPtr<model::MeshHeader>();
			numSub = pMesh->numSubMeshes;

			X_ASSERT(numSub > 0, "a areamodel can't have zero meshes")(numSub);

			// set meshHeads verts and faces.
			pMesh->subMeshHeads = cursor.postSeekPtr<model::SubMeshHeader>(numSub);

			// verts
			pMesh->streams[VertexStream::VERT] = cursor.postSeekPtr<uint8_t>(pMesh->numVerts *
				((sizeof(Vec2f)* 2) + sizeof(Vec3f)));
			pMesh->streams[VertexStream::COLOR] = cursor.postSeekPtr<Color8u>(pMesh->numVerts);
			pMesh->streams[VertexStream::NORMALS] = cursor.postSeekPtr<Vec3f>(pMesh->numVerts);

			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];
				pSubMesh->streams[VertexStream::VERT] += pMesh->streams[VertexStream::VERT];
				pSubMesh->streams[VertexStream::COLOR] += pMesh->streams[VertexStream::COLOR];
				pSubMesh->streams[VertexStream::NORMALS] += pMesh->streams[VertexStream::NORMALS];
			}

			// indexes
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];
				pSubMesh->indexes = cursor.postSeekPtr<model::Index>(pSubMesh->numIndexes);
			}

			// mat names
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];
				uint32_t matID = reinterpret_cast<uint32_t>(pSubMesh->materialName.as<uint32_t>());
				pSubMesh->materialName = stringTable_.getString(matID);
			}

			// load materials
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];

				pSubMesh->pMat = pMaterialManager_->loadMaterial(pSubMesh->materialName);
			}

			// set the mesh head pointers.
			pMesh->indexes = pMesh->subMeshHeads[0]->indexes;

			meshName.clear();
			meshName.appendFmt("$area_mesh%i", i);

			// update area info.
			Area& area = areas_[i];
			area.areaNum = i;
			area.pMesh = pMesh;
			area.pRenderMesh = gEnv->pRender->createRenderMesh(pMesh,
				shader::VertexFormat::P3F_T4F_C4B_N3F, meshName.c_str());
		
			// upload to gpu now.
			area.pRenderMesh->uploadToGpu();
		}

		if (!cursor.isEof()) {
			X_WARNING("Level", "potential read error, cursor is not at end. bytes left: %i",
				cursor.numBytesRemaning());
		}
	}

	// area Portals
	if (fileHdr_.numinterAreaPortals > 0)
	{
		core::XFileBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::AREA_PORTALS);

		// 2 ints for the area numbers followed by a winding.
		uint32_t i, numIaps = fileHdr_.numinterAreaPortals;

		for (i = 0; i < numIaps; i++)
		{		
			int32_t a1, a2;

			file.readObj(a1);
			file.readObj(a2);

			// check the area numbers are valid.
			if (a1 < 0 || a1 > fileHdr_.numAreas) {
				X_ERROR("Level","Invalid interarea area: %i valid-range(0,%i)",
					a1,fileHdr_.numAreas);
				return false;
			}
			if (a2 < 0 || a2 > fileHdr_.numAreas) {
				X_ERROR("Level", "Invalid interarea area: %i valid-range(0,%i)",
					a2, fileHdr_.numAreas);
				return false;
			}

			XWinding* pWinding = X_NEW(XWinding, g_3dEngineArena, "AreaPortalWinding");
			if (!pWinding->SLoad(&file)) {
				X_ERROR("Level", "Failed to load area windings");
				X_DELETE(pWinding, g_3dEngineArena);
				return false;
			}

			// add to areas
			Area& area1 = areas_[a1];
			Area& area2 = areas_[a2];

			AreaPortal& p1 = area1.portals.AddOne();
			AreaPortal& p2 = area2.portals.AddOne();

			// get p1's plane from the winding.
			pWinding->getPlane(p1.plane);
			p1.pWinding = pWinding;
			p1.areaTo = a2;

			p2.pWinding = pWinding->ReverseWinding();
			// p2's plane is made from the reverse winding.
			p2.pWinding->getPlane(p2.plane);
			p2.areaTo = a1;
		}

		if (!file.isEof()) {
			X_WARNING("Level", "potential read error, "
				"failed to process all bytes for node %i. bytes left: %i",
				FileNodes::AREA_PORTALS, file.remainingBytes());
		}
	}
	else
	{
		X_WARNING("Level","Level has no inter area portals.");
	}

	// nodes
	if (fileHdr_.flags.IsSet(LevelFileFlags::BSP_TREE))
	{
		core::XFileBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::BSP_TREE);

		areaNodes_.resize(fileHdr_.numNodes);

		// we read: nodeNumber, plane, childid's
		int32_t i;

		for (i = 0; i < fileHdr_.numNodes; i++)
		{
			AreaNode& node = areaNodes_[i];

			file.readObj(node.plane);
			file.readObj(node.children);
		}

		if (!file.isEof()) {
			X_WARNING("Level", "potential read error, "
				"failed to process all bytes for node %i. bytes left: %i",
				FileNodes::BSP_TREE, file.remainingBytes());
		}
	}
	else
	{
		X_WARNING("Level", "Level has no area tree.");
	}

	// 
	CommonChildrenArea_r(&areaNodes_[0]);


	// clean up.
	pFileSys_->closeFileAsync(pAsyncLoadData_->pFile_);

	X_DELETE_AND_NULL(pAsyncLoadData_, g_3dEngineArena);

	// stats.
	loadStats_.elapse = pTimer_->GetTimeReal() - loadStats_.startTime;

	// safe to render?
	canRender_ = true;

	X_LOG0("Level", "%s loaded in %gms", 
		path_.fileName(),
		loadStats_.elapse.GetMilliSeconds());

	return true;
}

int32_t Level::CommonChildrenArea_r(AreaNode* pAreaNode)
{
	int32_t	nums[2];

	for ( int i = 0 ; i < 2 ; i++ )
	{
		if (pAreaNode->children[i] <= 0) {
			nums[i] = -1 - pAreaNode->children[i];
		} else {
			nums[i] = CommonChildrenArea_r(&areaNodes_[pAreaNode->children[i]]);
		}
	}

	// solid nodes will match any area
	if (nums[0] == AreaNode::AREANUM_SOLID) {
		nums[0] = nums[1];
	}
	if (nums[1] == AreaNode::AREANUM_SOLID) {
		nums[1] = nums[0];
	}

	int32_t	common;
	if ( nums[0] == nums[1] ) {
		common = nums[0];
	} else {
		common = AreaNode::CHILDREN_HAVE_MULTIPLE_AREAS;
	}

	pAreaNode->commonChildrenArea = common;

	return common;
}


size_t Level::NumAreas(void) const
{
	return areas_.size();
}

size_t Level::NumPortalsInArea(int32_t areaNum) const
{
	X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t,size_t>(NumAreas())),
		"areaNum out of range")(areaNum, NumAreas());

	return areas_[areaNum].portals.size();
}

bool Level::IsPointInAnyArea(const Vec3f& pos) const
{
	int32_t areaOut;
	return IsPointInAnyArea(pos, areaOut);
}

bool Level::IsPointInAnyArea(const Vec3f& pos, int32_t& areaOut) const
{
	if (areaNodes_.isEmpty()) {
		areaOut = -1;
		return false;
	}

	const AreaNode* pNode = &areaNodes_[0];
	int32_t nodeNum;

	while (1)
	{
		float dis = pNode->plane.distance(pos);

		if (dis > 0.f) {
			nodeNum = pNode->children[0];
		}
		else {
			nodeNum = pNode->children[1];
		}
		if (nodeNum == 0) {
			areaOut = -1; // in solid
			return false;
		}
		
		if (nodeNum < 0) 
		{
			nodeNum = (-1 - nodeNum);

			if (nodeNum >= safe_static_cast<int32_t,size_t>(areaNodes_.size())) {
				X_ERROR("Level", "area out of range, when finding point for area");
			}

			areaOut = nodeNum;
			return true;
		}

		pNode = &areaNodes_[nodeNum]; 
	}

	areaOut = -1;
	return false;
}


#if 0
bool Level::LoadFromFile(const char* filename)
{
	X_ASSERT_NOT_NULL(filename);

	core::fileModeFlags mode;
	FileHeader hdr;
	core::Path path;
	bool LumpsLoaded;

	mode.Set(core::fileMode::READ);
	mode.Set(core::fileMode::RANDOM_ACCESS); // the format allows for each lump to be in any order currently.

	core::zero_object(hdr);

	path.append(filename);
	path.setExtension(level::LVL_FILE_EXTENSION);

	LumpsLoaded = false;

	core::XFileMemScoped file;
	if (file.openFile(path.c_str(), mode))
	{
		file->readObj(hdr);

		// is this header valid m'lady?
		if (!hdr.isValid())
		{
			// this header is not valid :Z
			if (hdr.fourCC == LVL_FOURCC_INVALID)
			{
				X_ERROR("Bsp", "%s file is corrupt, please re-compile.", path.fileName());
			}
			else
			{
				X_ERROR("Bsp", "%s is not a valid bsp", path.fileName());
			}

			return false;
		}

		if (hdr.version != LVL_VERSION)
		{
			X_ERROR("Bsp", "%s has a invalid version. provided: %i required: %i",
				path.fileName(), hdr.version, LVL_VERSION);
			return false;
		}

		if (hdr.datasize <= 0)
		{
			X_ERROR("Bsp", "level file is empty");
			return false;
		}

		// require atleast one area.
		if(hdr.numAreas < 1)
		{
			X_ERROR("Bsp", "Level file has no areas");
			return false;
		}

		// copy some stuff.
		numAreas_ = hdr.numAreas;


		pFileData_ = X_NEW_ARRAY(uint8_t, hdr.datasize, g_3dEngineArena, "LevelBuffer");
		
		uint32_t bytesRead = file->read(pFileData_, hdr.datasize);
		if (bytesRead != hdr.datasize)
		{
			X_ERROR("Bsp", "failed to read level file. read: %i of %i",
				bytesRead, hdr.datasize);

			X_DELETE_ARRAY(pFileData_,g_3dEngineArena);
			pFileData_ = nullptr;
			return false;
		}

		core::StackString<64> meshName;
		core::MemCursor<uint8_t> cursor(pFileData_, hdr.datasize);
		uint32_t x, numSub;

		areaModels_.reserve(hdr.numAreas);
		for (uint32_t i = 0; i < hdr.numAreas; i++)
		{
			model::MeshHeader* pMesh = cursor.getSeekPtr<model::MeshHeader>();		
			numSub = pMesh->numSubMeshes;

			X_ASSERT(numSub > 0, "a areamodel can't have zero meshes")(numSub);

			// set meshHeads verts and faces.
			pMesh->subMeshHeads = cursor.postSeekPtr<model::SubMeshHeader>(numSub);
			
			// verts
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];
				pSubMesh->verts = cursor.postSeekPtr<level::Vertex>(pSubMesh->numVerts);;
			}

			// indexs
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];
				pSubMesh->indexes = cursor.postSeekPtr<model::Index>(pSubMesh->numIndexes);
			}

			// set the mesh head pointers.
			pMesh->streams[VertexStream::VERT] = pMesh->subMeshHeads[0]->verts;
			pMesh->indexes = pMesh->subMeshHeads[0]->indexes;

			meshName.clear();
			meshName.appendFmt("$area_mesh%i", i);

			AreaModel area;
			area.pMesh = pMesh;
			area.pRenderMesh = gEnv->pRender->createRenderMesh(pMesh, 
				shader::VertexFormat::P3F_T4F_C4B_N3F, meshName.c_str());
			area.pRenderMesh->uploadToGpu();

			areaModels_.append(area);
		}


		if(!cursor.isEof())
		{
			X_WARNING("Bsp", "potential read error, cursor is not at end");
		}


		file.close(); // not needed but makes my nipples more perky.
		return true;
	}

	return false;
}
#endif

X_NAMESPACE_END
