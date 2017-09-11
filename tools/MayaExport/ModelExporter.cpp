#include "stdafx.h"
#include "ModelExporter.h"


#include <maya\MItDag.h>
#include <maya\MFnMesh.h>
#include <maya\MPointArray.h>
#include <maya\MFloatVectorArray.h>
#include <maya\MFloatArray.h>
#include <maya\MFloatPointArray.h>
#include <maya\MDagPath.h>
#include <maya\MDagPathArray.h>
#include <maya\MSelectionList.h>
#include <maya\MItSelectionList.h>
#include <maya\MGlobal.h>
#include <maya\MPlug.h>
#include <maya\MFnSet.h>
#include <maya\MItMeshPolygon.h>
#include <maya\MItDependencyGraph.h>
#include <maya/MProgressWindow.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MMatrix.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItGeometry.h>
#include <maya/MArgList.h>
#include <maya/MFnTransform.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MFnNumericData.h>
#include <maya/MArgDatabase.h>
#include <maya\MSyntax.h>


X_DISABLE_WARNING(4702)
#include <algorithm>
#include <map>
X_ENABLE_WARNING(4702)

#include <String\StringTokenizer.h>
#include <Containers\ByteStream.h>
#include <Containers\FixedStack.h>

#include <Threading\Thread.h>
#include <String\HumanSize.h>
#include <Compression\LZ4.h>

#include "Profiler.h"
#include "MayaUtil.h"
#include "AssetDB.h"

X_USING_NAMESPACE;


// --------------------------------------------

MayaBone::MayaBone() : 
	dagnode(nullptr)
{
	mayaNode.setOwner(this);
}

MayaBone::MayaBone(MayaBone&& oth) : 
	mayaNode(std::move(oth.mayaNode))
{
	name = std::move(oth.name);
	dagnode = std::move(oth.dagnode);
	index = oth.index;

	bindpos = oth.bindpos;
	bindRotation = oth.bindRotation;

	mayaNode.setOwner(this);
}

MayaBone& MayaBone::operator = (MayaBone&& oth)
{
	name = std::move(oth.name);
	dagnode = std::move(oth.dagnode);
	index = oth.index;

	bindpos = oth.bindpos;
	bindRotation = oth.bindRotation;

	mayaNode = oth.mayaNode;
	mayaNode.setOwner(this);
	return *this;
}

// --------------------------------------------

ModelExporter::ModelExporter(core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena) :
	model::RawModel::Model(arena, pJobSys),
	scale_(1.f),
	mayaBones_(arena),
	exportMode_(ExpoMode::SERVER),
	meshExportMode_(MeshExpoMode::EXPORT_INPUT),
	unitOfMeasurement_(MDistance::Unit::kInches)
{
	tagOrigin_.index = 0;
	tagOrigin_.name.append("tag_origin");
	tagOrigin_.dagnode = nullptr;

	MayaUtil::MayaPrintMsg("=========== Exporting Model ===========");
}

ModelExporter::~ModelExporter()
{
	MayaUtil::HideProgressDlg();
	MayaUtil::MayaPrintMsg("================= End =================");
}


MStatus ModelExporter::getInputObjects(void)
{
	// we just want to load all objects for every lod.
	for (size_t i = 0; i < lodExpoInfo_.size(); i++)
	{
		MDagPathArray pathArry;
		MSelectionList list;
		LODExportInfo& info = lodExpoInfo_[i];
		
		MStringArray nameArr;
		info.objects.split(' ', nameArr);

		for (uint32_t x = 0; x < safe_static_cast<uint32_t>(nameArr.length()); x++) {
			list.add(nameArr[x]);
		}

		for (size_t x = 0; x < list.length(); x++)
		{
			MDagPath path;
			auto status = list.getDagPath(safe_static_cast<uint32_t>(x), path);
			if (status) {
				pathArry.append(path);
			}
			else {
				MayaUtil::MayaPrintError("getDagPath failed: %s", status.errorString().asChar());
			}
		}
		

		if (pathArry.length() < 1) {
			return MS::kFailure;
		}

		info.exportObjects = pathArry;
	}

	return MS::kSuccess;
}

MStatus ModelExporter::getExportObjects(void)
{
	if (meshExportMode_ == MeshExpoMode::EXPORT_INPUT){
		return getInputObjects();
	} else {
		MayaUtil::MayaPrintWarning("Exporting selection or all is not currently supported");
	}

	return MS::kFailure;
}


MStatus ModelExporter::convert(const MArgList& args)
{
	PROFILE_MAYA_NAME("Export Time");

	MStatus status;

	status = parseArgs(args);
	if (status != MS::kSuccess) {
		MayaUtil::MayaPrintError("Failed to parse arguments");
		return status;
	}

	status = MayaUtil::ShowProgressDlg(0, (exportMode_ == ExpoMode::RAW) ? 5 : 7);
	bool saveOk = false;

	// name length check
	if (name_.length() > model::MODEL_MAX_NAME_LENGTH)
	{
		MayaUtil::MayaPrintError("Model name is too long. MAX: %" PRIu32 ", provided: %" PRIuS,
			model::MODEL_MAX_NAME_LENGTH, name_.length());
		return MS::kFailure;;
	}

	{
		MayaUtil::SetProgressText("Loading export list");

		status = getExportObjects();

		if (!status) {
			MayaUtil::MayaPrintError("Failed to collect export objects from maya: %s", status.errorString().asChar());
			return status;
		}


		MayaUtil::SetProgressText("Loading joints");
		status = loadBones();
		if (!status) {
			MayaUtil::MayaPrintError("Failed to load joints: %s", status.errorString().asChar());
			return status;
		}


		MayaUtil::SetProgressText("Loading mesh data");

		{
			PROFILE_MAYA_NAME("Load data");

			// load them baby
			status = loadLODs();
		}

		if (!status) {
			MayaUtil::MayaPrintError("Failed to load meshes: %s", status.errorString().asChar());
			return status;
		}


		// time to slap a goat!
		if (exportMode_ == ExpoMode::RAW)
		{
			PROFILE_MAYA_NAME("Save Raw");

			core::Path<char> outPath = getFilePath();
			{
				MayaUtil::MayaPrintMsg("Exporting to: '%s'", outPath.c_str());
				MayaUtil::MayaPrintMsg(""); // new line
			}

			MayaUtil::SetProgressText("Saving raw model");

			if (!SaveRawModel(outPath)) {
				MayaUtil::MayaPrintError("Failed to save raw model");
				return MS::kFailure;
			}
		}
		else if (exportMode_ == ExpoMode::SERVER)
		{
			MayaUtil::SetProgressText("Getting info from server");

			maya::AssetDB::ConverterInfo info;
			if (!maya::AssetDB::Get()->GetConverterInfo(info)) {
				X_ERROR("Model", "Failed to get info from server");
				return status;
			}

			int32_t assetId, modId;
			status = maya::AssetDB::Get()->AssetExsists(maya::AssetDB::AssetType::MODEL, MString(getName()), &assetId, &modId);
			if (!status) {
				X_ERROR("Model", "Failed to get meta from server");
				return status;
			}

			if (assetId == assetDb::INVALID_ASSET_ID || modId == assetDb::INVALID_MOD_ID) {
				X_ERROR("Model", "Asset is not registerd with server");
				return MS::kFailure;
			}

			maya::AssetDB::Mod mod;
			if (!maya::AssetDB::Get()->GetModInfo(modId, mod)) {
				X_ERROR("Model", "Failed to get mod info from server");
				return status;
			}

			MayaUtil::SetProgressText("Saving raw model");

			core::Array<uint8_t> compressed(g_arena);

			{
				PROFILE_MAYA_NAME("Save Raw");

				core::Array<uint8_t> rawModel(g_arena);

				if (!SaveRawModel(rawModel)) {
					MayaUtil::MayaPrintError("Failed to save raw model");
					return MS::kFailure;
				}

				PROFILE_MAYA_NAME("Deflate Raw");

				core::Compression::Compressor<core::Compression::LZ4> comp;

				if (!comp.deflate(g_arena, rawModel, compressed, core::Compression::CompressLevel::HIGH))
				{
					X_ERROR("Model", "Failed to defalte raw model");
					return MS::kFailure;
				}
				else
				{
					core::HumanSize::Str sizeStr, sizeStr2;
					X_LOG0("Model", "Defalated %s -> %s",
						core::HumanSize::toString(sizeStr, rawModel.size()),
						core::HumanSize::toString(sizeStr2, compressed.size()));
				}

			}

			{
				MayaUtil::SetProgressText("Syncing data to AssetServer");

				PROFILE_MAYA_NAME("Sync To Server");

				// send to asset server.
				status = maya::AssetDB::Get()->UpdateAsset(maya::AssetDB::AssetType::MODEL,
					MString(getName()),
					compressed
				);

				if (!status) {
					X_ERROR("Model", "Failed update AssetDB");
					return status;
				}
			}

		}
		else {
			MayaUtil::IncProcess();
		}

		saveOk = true;

		MayaUtil::SetProgressText("Complete");
		printStats();
	}

	if (saveOk) {
		status.setSuccess();
	}
	else {
		MayaUtil::MayaPrintError("Failed to write file");
		return MS::kFailure;
	}
	
	return status;
}

void ModelExporter::printStats(void) const
{
	core::StackString<2048> info;

	const char* pUOMStr = "centimeters";
	switch (unitOfMeasurement_)
	{
		case MDistance::kInches:
			pUOMStr = "inches";
			break;
		case MDistance::kFeet:
			pUOMStr = "feet";
			break;
		case MDistance::kYards:
			pUOMStr = "yards";
			break;
		case MDistance::kMiles:
			pUOMStr = "miles";
			break;
		case MDistance::kMillimeters:
			pUOMStr = "millimeters";
			break;
		case MDistance::kCentimeters:
			pUOMStr = "centimeters";
			break;
		case MDistance::kKilometers:
			pUOMStr = "kilometers";
			break;
		case MDistance::kMeters:
			pUOMStr = "meters";
			break;

	}

	info.append("\nModel Info:");
	info.appendFmt("\n> Units: %s", pUOMStr);
	info.appendFmt("\n> Scale: %f", scale_);
	info.appendFmt("\n> Total Lods: %" PRIuS, lods_.size());
	info.appendFmt("\n> Total Bones: %" PRIuS, bones_.size());

	info.append(" (");
	for (auto& bone : bones_)
	{
		info.appendFmt("%s,", bone.name_.c_str());
	}
	info.trimRight(',');
	info.append(")");

	for (auto& lod : lods_)
	{
		info.appendFmt("\n> LOD");
		info.appendFmt("\n > Total Mesh: %" PRIuS, lod.numMeshes());
		info.appendFmt("\n > Total Verts: %" PRIuS, lod.totalVerts());
		info.appendFmt("\n > Total Faces: %" PRIuS, lod.totalTris());
		info.append("\n");
	}

	MayaUtil::MayaPrintMsg(info.c_str());
}

void ModelExporter::setFileName(const MString& path)
{
	core::StackString512 temp(path.asChar());
	temp.trim();
	// we don't replace seperators on the name as asset names 
	// have a fixed slash, regardless of native slash of the platform.
	// so we replace slashes only when building file paths.

	name_ = temp.c_str();
	fileName_.set(temp.c_str());
	fileName_.setExtension(model::MODEL_FILE_EXTENSION);
}

void ModelExporter::setOutdir(const MString& path)
{
	core::StackString<512, char> temp(path.asChar());
	temp.trim();

	outDir_.set(temp.c_str());
	outDir_.replaceSeprators();
}

core::Path<char> ModelExporter::getFilePath(void) const
{
	core::Path<char> path(outDir_);
	path /= fileName_;
	path.replaceSeprators();
	return path;
}

const core::string& ModelExporter::getName(void) const
{
	return name_;
}

MStatus ModelExporter::parseArgs(const MArgList& args)
{
	MStatus status;
	uint32_t idx;

	// required
	{
		idx = args.flagIndex("f");
		if (idx == MArgList::kInvalidArgIndex) {
			MayaUtil::MayaPrintError("missing file argument");
			return MS::kFailure;
		}

		MString Mfilename;

		status = args.get(++idx, Mfilename);
		if (!status) {
			MayaUtil::MayaPrintError("missing filename");
			return status;
		}

		setFileName(Mfilename);
	}

	idx = args.flagIndex("scale");
	if (idx != MArgList::kInvalidArgIndex) {
		double temp;
		if (!args.get(++idx, temp)) {
			MayaUtil::MayaPrintWarning("failed to get scale flag");
		}
		else {
			scale_ = static_cast<float>(temp);
		}
	}
	else {
		scale_ = 1.f;
	}


	// allow for options to be parsed.
	idx = args.flagIndex("verbose");
	if (idx != MArgList::kInvalidArgIndex) {
		MayaUtil::SetVerbose(true);
	}
	else {
		MayaUtil::SetVerbose(false);
	}

	idx = args.flagIndex("mode");
	if (idx != MArgList::kInvalidArgIndex) {
		MString modeStr;
		if (!args.get(++idx, modeStr)) {
			MayaUtil::MayaPrintWarning("failed to get export mode");
		}
		else {
			if (core::strUtil::IsEqualCaseInsen(modeStr.asChar(), "Raw")) {
				exportMode_ = ExpoMode::RAW;
			}
			else if (core::strUtil::IsEqualCaseInsen(modeStr.asChar(), "Server")) {
				exportMode_ = ExpoMode::SERVER;
			} else {
				MayaUtil::MayaPrintWarning("Unknown export mode: \"%s\"",
					modeStr.asChar());
			}
		}
	}
	else {
		exportMode_ = ExpoMode::SERVER;
	}

	idx = args.flagIndex("use_cm");
	if (idx != MArgList::kInvalidArgIndex) {
		bool useCm = false;
		if (!args.get(++idx, useCm)) {
			MayaUtil::MayaPrintWarning("failed to get use_cm flag");
		}
		else {
			if (useCm) {
				unitOfMeasurement_ = MDistance::Unit::kCentimeters;
			}
		}
	}

	idx = args.flagIndex("dir");
	if (idx != MArgList::kInvalidArgIndex) {
		MString dir;
		if (!args.get(++idx, dir)) {
			MayaUtil::MayaPrintWarning("failed to get dir flag");
		}
		else {
			setOutdir(dir);
		}
	}

	idx = args.flagIndex("force_bones");
	if (idx != MArgList::kInvalidArgIndex) {
		MString forceBoneFilters;
		if (!args.get(++idx, forceBoneFilters)) {
			MayaUtil::MayaPrintWarning("failed to get force_bones flag");
		}
		// TODO: use the forceBoneFilters.
	}

	{
		MString progressCntl;

		idx = args.flagIndex("progress");
		if (idx != MArgList::kInvalidArgIndex) {
			if (!args.get(++idx, progressCntl)) {
				MayaUtil::MayaPrintWarning("failed to get progress cntl flag");
			}
		}

		MayaUtil::SetProgressCtrl(progressCntl);
	}

	{
		uint32_t currentLod = 0;

		// get the LOD options.
		for (uint32_t i = 0; i < model::MODEL_MAX_LODS; i++)
		{
			core::StackString<32> lodName;
			lodName.appendFmt("LOD%" PRIu32, i);
			idx = args.flagIndex(lodName.c_str());
			if (idx != MArgList::kInvalidArgIndex)
			{
				double temp;
				LODExportInfo info;
				info.idx = currentLod;
				if (!args.get(++idx, temp) ||
					!args.get(++idx, info.objects)) {
					MayaUtil::MayaPrintError("failed to parse LOD%" PRIu32 " info", currentLod);
					continue;
				}
				if (lodExpoInfo_.size() == model::MODEL_MAX_LODS) {
					MayaUtil::MayaPrintError("Exceeded max load count: %" PRIu32 , model::MODEL_MAX_LODS);
					return MS::kFailure;
				}

				info.distance = static_cast<float>(temp);
				lodExpoInfo_.append(info);

				currentLod++;
			}
		}

		//	MayaPrintMsg("Total LOD: %" PRIu32, currentLod);
		if (currentLod == 0) {
			MayaUtil::MayaPrintError("Failed to parse and LOD info");
			return MS::kFailure;
		}
	}

	return status;
}


MStatus ModelExporter::loadLODs(void)
{
	MStatus status;

	lods_.resize(lodExpoInfo_.size(), model::RawModel::Lod(g_arena));

	for (size_t i = 0; i < lodExpoInfo_.size(); i++)
	{
		LODExportInfo& info = lodExpoInfo_[i];
		uint32_t numMesh = info.exportObjects.length();

		if (numMesh == 0) {
			MayaUtil::MayaPrintError("LOD%" PRIuS " failed to load meshes for lod no meshes found.", i);
			return MS::kFailure;
		}
		
		model::RawModel::Lod& lod = lods_[i];

		// reserver the meshes.
		lod.meshes_.resize(numMesh, model::RawModel::Mesh(g_arena));

		MFloatPointArray	vertexArray;
		MFloatVectorArray	normalsArray, tangentsArray, binormalsArray;
		MStringArray		UVSets;
		MFloatArray			u, v;
		MPointArray			points;
		MIntArray			vertexList;
		MColorArray			colorsArray;

		for (uint32_t meshIdx = 0; meshIdx < numMesh; meshIdx++)
		{
			MFnMesh fnmesh(info.exportObjects[meshIdx], &status);

			if (!status) {
				continue;
			}

			model::RawModel::Mesh& mesh = lod.meshes_[meshIdx];
			model::RawModel::Material& material = mesh.material_;

			mesh.displayName_ = getMeshDisplayName(fnmesh.fullPathName());
			mesh.name_.set(fnmesh.name().asChar());

			if (isColisionMesh(mesh.name_)) {
				hasColisionMeshes_ = true;
				MayaUtil::MayaPrintVerbose("\n ------ Processing (%s : Collision) -------", mesh.displayName_.c_str());
			}
			else {
				MayaUtil::MayaPrintVerbose("\n ------ Processing (%s) -------", mesh.displayName_.c_str());
			}

			if (!getMeshMaterial(info.exportObjects[meshIdx], material)) {
				MayaUtil::MayaPrintError("Mesh(%s): failed to get material", fnmesh.name().asChar());
				return MS::kFailure;
			}

			MayaUtil::MayaPrintVerbose("Material: '%s'", material.name_.c_str());


			int32_t numVerts = fnmesh.numVertices(&status);
			int32_t numPoly = fnmesh.numPolygons(&status);

			if (fnmesh.numUVSets() < 1) {
				MayaUtil::MayaPrintError("Mesh(%s): has no uv sets: %s", fnmesh.name().asChar());
				return MS::kFailure;
			}

			if (!status) {
				MayaUtil::MayaPrintError("Mesh(%s): failed to get mesh info (%s)", fnmesh.name().asChar(), status.errorString().asChar());
				return status;
			}

			MayaUtil::MayaPrintVerbose("NumVerts: %" PRIi32, numVerts);
			MayaUtil::MayaPrintVerbose("NumPoly: %" PRIi32, numPoly);

			// resize baby.
			mesh.verts_.resize(numVerts);
			mesh.tris_.reserve(numPoly);
			mesh.tris_.setGranularity(numPoly);

			// need to be cleared.
			UVSets.clear();

			status = fnmesh.getUVSetNames(UVSets);
			if (!status) {
				MayaUtil::MayaPrintError("Mesh(%s): failed to get UV set names (%s)",
					fnmesh.name().asChar(), status.errorString().asChar());
				return status;
			}

			// print how many :Z
			MayaUtil::MayaPrintVerbose("NumUvSets: %" PRIu32, UVSets.length());
			for (uint32_t x = 0; x < UVSets.length(); x++) {
				MayaUtil::MayaPrintVerbose("-> Set(%" PRIu32 "): %s", x, UVSets[static_cast<uint32_t>(i)].asChar());
			}

			MString uvSet;
			status = fnmesh.getCurrentUVSetName(uvSet);
			if (!status) {
				MayaUtil::MayaPrintError("Mesh(%s): failed to get current UV set (%s)",
					fnmesh.name().asChar(), status.errorString().asChar());

				if (UVSets.length() < 1) {
					return status;
				}

				uvSet = UVSets[0];

				MayaUtil::MayaPrintWarning("Falling back to uv set: %s", uvSet.asChar());
			}
			else
			{
				MayaUtil::MayaPrintVerbose("Default uv set: %s", uvSet.asChar());
			}


			vertexArray.clear();
			normalsArray.clear();
			tangentsArray.clear();
			binormalsArray.clear();
			u.clear();
			v.clear();
			colorsArray.clear();

			{
				using std::cerr;
				using std::endl;
				CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getUVs(u, v, &uvSet));
				CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getPoints(vertexArray, MSpace::kWorld));
				CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getNormals(normalsArray, MSpace::kWorld));
				CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getTangents(tangentsArray, MSpace::kWorld));
				CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getBinormals(binormalsArray, MSpace::kWorld));

				if (fnmesh.numColorSets() > 0) {

					MStringArray colorSetNames;
					CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getColorSetNames(colorSetNames));
					MColor defaultCol(1.f,1.f,1.f,1.f);

#if MAYA_API_VERSION == 850
					CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getColors(colorsArray, nullptr));
#else
					CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getColors(colorsArray, nullptr, &defaultCol));
#endif // !MAYA_SDK
				}
			}
			
			MayaUtil::MayaPrintVerbose("u: %" PRIu32 " v: %" PRIu32, u.length(), v.length());
			MayaUtil::MayaPrintVerbose("VertexArray: %" PRIu32, vertexArray.length());
			MayaUtil::MayaPrintVerbose("NormalsArray: %" PRIu32, normalsArray.length());
			MayaUtil::MayaPrintVerbose("TangentsArray: %" PRIu32, tangentsArray.length());
			MayaUtil::MayaPrintVerbose("BinormalsArray: %" PRIu32, binormalsArray.length());
			MayaUtil::MayaPrintVerbose("ColorsArray: %" PRIu32, colorsArray.length());

			// verts
			if (scale_ == 1.f) {
				for (int32_t x = 0; x < numVerts; x++) {
					model::RawModel::Vert& vert = mesh.verts_[x];
					vert.pos_ = MayaUtil::ConvertToGameSpace(MayaUtil::XVec(vertexArray[x]));
				}
			}
			else 
			{
				float scale = scale_;
				for (int32_t x = 0; x < numVerts; x++) {
					model::RawModel::Vert& vert = mesh.verts_[x];
					vert.pos_ = MayaUtil::ConvertToGameSpace(MayaUtil::XVec(vertexArray[x]));
					vert.pos_ *= scale;
				}
			}

			MIntArray polygonVertices;
			core::FixedArray<uint32_t, 8> localIndex;

			// for each polygon we might have multiple triangles.
			MItMeshPolygon itPolygon(info.exportObjects[meshIdx], MObject::kNullObj);

			for (; !itPolygon.isDone(); itPolygon.next())
			{
				int32_t numTriangles;

				itPolygon.numTriangles(numTriangles);
				itPolygon.getVertices(polygonVertices);

				while (numTriangles--)
				{
					status = itPolygon.getTriangle(numTriangles, points, vertexList, MSpace::kObject);

					// triangles!
					if (points.length() != 3) {
						continue;
					}

					getLocalIndex(polygonVertices, vertexList, localIndex);

					model::RawModel::Tri& tri = mesh.tris_.AddOne();

					for (uint32_t t = 0; t < 3; t++)
					{
						int32_t index = vertexList[t];
						int32_t uvID, colIdx;

						itPolygon.getUVIndex(localIndex[t], uvID, &uvSet);
						// fucking maya make your mind up about signed or unsiged index's
						uint32_t normalIdx = itPolygon.normalIndex(localIndex[t]);


						model::RawModel::TriVert& face = tri[t];

						face.index_ = index;
						face.normal_ = MayaUtil::XVec(normalsArray[normalIdx]);
						face.tangent_ = MayaUtil::XVec(tangentsArray[normalIdx]);
						face.biNormal_ = MayaUtil::XVec(binormalsArray[normalIdx]);
						face.uv_.x = u[uvID];
						face.uv_.y = 1.f - v[uvID]; // flip a camel

						if (colorsArray.length() > 0 &&	itPolygon.getColorIndex(t, colIdx, nullptr) == MS::kSuccess)
						{
							face.col_ = MayaUtil::XVec(colorsArray[colIdx]);
						}
						else
						{
							face.col_ = Color(1.f, 1.f, 1.f, 1.f);
						}
					}
				}
			}

			// weights

			//search the skin cluster affecting this geometry
			MItDependencyNodes kDepNodeIt(MFn::kSkinClusterFilter);

			// Get any attached skin cluster
			bool hasSkinCluster = false;

			// Go through each skin cluster in the scene until we find the one connected to this mesh.
			for (; !kDepNodeIt.isDone() && !hasSkinCluster; kDepNodeIt.next())
			{
				MObject kObject = kDepNodeIt.item();
				MFnSkinCluster kSkinClusterFn(kObject, &status);
				if (status != MS::kSuccess) {
					continue;
				}

				uint32_t uiNumGeometries = kSkinClusterFn.numOutputConnections();

				// go through each connection on the skin cluster until we get the one connecting to this mesh.
				// pretty sure maya api dose not let me get this info without checking.
				// might be wrong.
				for (uint32_t uiGeometry = 0; uiGeometry < uiNumGeometries && !hasSkinCluster; ++uiGeometry)
				{
					uint32_t uiIndex = kSkinClusterFn.indexForOutputConnection(uiGeometry, &status);

					MObject kInputObject = kSkinClusterFn.inputShapeAtIndex(uiIndex, &status);
					MObject kOutputObject = kSkinClusterFn.outputShapeAtIndex(uiIndex, &status);

					if (kOutputObject == fnmesh.object())
					{
						hasSkinCluster = true;

						MDagPath path = info.exportObjects[meshIdx];
						MDagPathArray infs;
						MFloatArray wts;
						uint32_t nInfs, infCount;

						nInfs = kSkinClusterFn.influenceObjects(infs, &status);

						if (!status) {
							MayaUtil::MayaPrintError("Mesh '%s': Error getting influence objects (%s)",
								mesh.displayName_.c_str(), status.errorString().asChar());
							continue;
						}


						// work out bones indexes.
						core::Array<MayaBone*> joints(g_arena);
						joints.reserve(nInfs);

						for (uint32_t x = 0; x < nInfs; ++x) 
						{
							const char* pName;
							MString s;
							MayaBone* pJoint;

							s = infs[x].partialPathName();
							pName = s.asChar();

							pJoint = findJointReal(pName);
							if (!pJoint) {
								MayaUtil::MayaPrintError("Mesh '%s': joint %s not found", mesh.displayName_.c_str(), pName);
							}

							joints.append(pJoint);
						}


						MItGeometry kGeometryIt(kInputObject);
						for (; !kGeometryIt.isDone(); kGeometryIt.next())
						{
							MObject comp = kGeometryIt.component(&status);
							if (!status) {
								MayaUtil::MayaPrintError("Mesh '%s': Error getting component (%s)", 
									mesh.displayName_.c_str(), status.errorString().asChar());
								continue;
							}

							// Get the weights for this vertex (one per influence object)
							status = kSkinClusterFn.getWeights(path, comp, wts, infCount);

							if (!status) {
								MayaUtil::MayaPrintError("Mesh '%s': Error getting weights (%s)", 
									mesh.displayName_.c_str(), status.errorString().asChar());
								continue;
							}
							if (0 == infCount) {
								MayaUtil::MayaPrintError("Mesh '%s': Error: 0 influence objects.", 
									mesh.displayName_.c_str());
								continue;
							}


							int32_t idx = kGeometryIt.index();	
							model::RawModel::Vert& vert = mesh.verts_[idx];

							for (uint32_t x = 0; x < infCount; x++)
							{
								float w = static_cast<float>(wts[x]);

								if (w > EPSILON_VALUEf)
								{
									MayaBone* pBone = joints[x];

									// full?
									if (vert.binds_.size() == vert.binds_.capacity())
									{
										size_t smallest = std::numeric_limits<size_t>::max();
										for (size_t j = 0; j < vert.binds_.size(); j++)
										{
											const float checkWeight = vert.binds_[j].weight_;
											if (checkWeight < w) {
												smallest = j; // save the index.
											}
										}

										if (smallest < std::numeric_limits<size_t>::max()) {
											model::RawModel::Bind& bind = vert.binds_[smallest];
											bind.weight_ = w;
											bind.boneIdx_ = pBone->index;
										}
										else {
											// don't add
										}
									}
									else
									{
										model::RawModel::Bind& bind = vert.binds_.AddOne();
										bind.weight_ = w;
										bind.boneIdx_ = pBone->index;
									}
								}
							}
						}
					}
				}
			}

			if (!hasSkinCluster) {
				MayaUtil::MayaPrintWarning("No bindInfo found for mesh: '%s' binding to root.",
					mesh.displayName_.c_str());
			}
		}
	}

	return status;
}

MStatus ModelExporter::loadBones(void)
{
	MStatus			status;
	MDagPath		dagPath;

	// allocate max.
	mayaBones_.reserve(model::MODEL_MAX_BONES);

	MItDag dagIterator(MItDag::kBreadthFirst, MFn::kTransform, &status);
	for (; !dagIterator.isDone(); dagIterator.next()) 
	{
		status = dagIterator.getPath(dagPath);
		if (!status) {
			MayaUtil::MayaPrintError("Joint: MItDag::getPath failed (%s)", status.errorString().asChar());
			return status;
		}

		if (!dagPath.hasFn(MFn::kJoint)) {
			continue;
		}

		MayaBone new_bone;
		new_bone.dagnode = core::makeUnique<MFnDagNode>(g_arena, dagPath, &status);

		if (!status) {
			MayaUtil::MayaPrintError("Joint: MFnDagNode constructor failed (%s)", status.errorString().asChar());
			return status;
		}

		new_bone.name.append(new_bone.dagnode->name().asChar());
		if (mayaBones_.size() == model::MODEL_MAX_BONES) {
			MayaUtil::MayaPrintError("Joints: max bones reached: %" PRIu32, model::MODEL_MAX_BONES);
			return MStatus::kFailure;
		}

		if (new_bone.name.length() > model::MODEL_MAX_BONE_NAME_LENGTH) {
			MayaUtil::MayaPrintError("Joints: max pMayaBone name length exceeded, MAX: %" PRIu32 " '%s' -> %" PRIuS,
				model::MODEL_MAX_BONE_NAME_LENGTH, new_bone.name.c_str(), new_bone.name.length());
			return MStatus::kFailure;
		}

		new_bone.index = safe_static_cast<int32_t>(mayaBones_.size());
		mayaBones_.append(std::move(new_bone));
	}


	// create hierarchy
	for (size_t i = 0; i < mayaBones_.size(); i++) 
	{
		auto& mayaBone = mayaBones_[i];

		if (!mayaBone.dagnode) {
			continue;
		}

		mayaBone.mayaNode.setParent(mayaHead_);

		auto parentNode = getParentBone(mayaBone.dagnode.get());
		if (parentNode) 
		{
			// do we have this joint?
			for (size_t j = 0; j < mayaBones_.size(); j++) {
				if (!mayaBones_[j].dagnode) {
					continue;
				}

				if (mayaBones_[j].dagnode->name() == parentNode->name()) {
					mayaBone.mayaNode.setParent(mayaBones_[j].mayaNode);
					break;
				}
			}
		}

		status = getBindPose(mayaBone);
		if (!status) {
			return status;
		}
	}

	// fill in the raw bones.
	{
		bones_.resize(mayaBones_.size());
		for (size_t i = 0; i < mayaBones_.size(); i++)
		{
			const auto& mayaBone = mayaBones_[i];
			auto& bone = bones_[i];

			MayaBone* pParent = mayaBone.mayaNode.parent();
			if (pParent)
			{
				bone.parIndx_ = pParent->index;
			}
			else
			{
				bone.parIndx_ = -1;
			}

			Vec3f pos = ConvertUnitOfMeasure(mayaBone.bindpos);
			pos *= scale_;

			bone.name_ = mayaBone.name.c_str();
			bone.rotation_ = mayaBone.bindRotation;
			bone.worldPos_ = pos;
			bone.scale_ = mayaBone.scale;
		}
	}

	return status;
}


X_INLINE double ModelExporter::ConvertUnitOfMeasure(double value)
{
	MDistance d(value);
	return d.as(unitOfMeasurement_);
}

X_INLINE Vec3d ModelExporter::ConvertUnitOfMeasure(const Vec3d& vec)
{
	Vec3d ret;
	ret.x = ConvertUnitOfMeasure(vec.x);
	ret.y = ConvertUnitOfMeasure(vec.y);
	ret.z = ConvertUnitOfMeasure(vec.z);
	return ret;
}

X_INLINE Vec3f ModelExporter::ConvertUnitOfMeasure(const Vec3f& vec)
{
	Vec3f ret;
	ret.x = static_cast<float>(ConvertUnitOfMeasure(vec.x));
	ret.y = static_cast<float>(ConvertUnitOfMeasure(vec.y));
	ret.z = static_cast<float>(ConvertUnitOfMeasure(vec.z));
	return ret;
}

MayaBone* ModelExporter::findJointReal(const char* pName)
{
	MayaBone* pBone = mayaBones_.ptr();
	for (size_t i = 0; i < mayaBones_.size(); i++, pBone++) {
		if (pBone->name.isEqual(pName)) {
			return pBone;
		}
	}
	return nullptr;
}


void ModelExporter::getLocalIndex(MIntArray& getVertices, MIntArray& getTriangle, core::FixedArray<uint32_t, 8>& indexOut)
{
	indexOut.clear();

	for (uint32_t gt = 0; gt < getTriangle.length(); gt++)
	{
		for (uint32_t gv = 0; gv < getVertices.length(); gv++)
		{
			if (getTriangle[gt] == getVertices[gv])
			{
				indexOut.append(gv);
				break;
			}
		}
	}
}

core::UniquePointer<MFnDagNode> ModelExporter::getParentBone(MFnDagNode* pBone)
{
	X_ASSERT_NOT_NULL(pBone);

	MStatus		status;
	MObject		parentObject;

	core::UniquePointer<MFnDagNode> node;

	parentObject = pBone->parent(0, &status);
	if (!status && status.statusCode() == MStatus::kInvalidParameter) {
		return node;
	}

	while (!parentObject.hasFn(MFn::kTransform)) {
		MFnDagNode parentNode(parentObject, &status);
		if (!status) {
			return node;
		}

		parentObject = parentNode.parent(0, &status);
		if (!status && status.statusCode() == MStatus::kInvalidParameter) {
			return node;
		}
	}

	node = core::makeUnique<MFnDagNode>(g_arena, parentObject, &status); 
	if (!status) {
		node.reset();
	}

	return node;
}


MStatus ModelExporter::getBindPose(MayaBone& bone)
{
	MStatus	status;
	MDagPath dagPath;

	status = bone.dagnode->getPath(dagPath);
	if (!status) {
		return status;
	}
	
	const MObject jointNode = dagPath.node(&status);
	if (!status) {
		return status;
	}

	Matrix33f worldMatrix;
	Vec3f worldPos;


	// look for bind pose
	bool foundBindPose = false;

	{
		MFnDependencyNode fnJoint(jointNode);
		MObject	aBindPose = fnJoint.attribute("bindPose", &status);

		if (status)
		{
			MPlugArray	connPlugs;
			MPlug		pBindPose(jointNode, aBindPose);

			pBindPose.connectedTo(connPlugs, false, true);
			for (uint32_t i = 0; i < connPlugs.length(); ++i)
			{
				if (connPlugs[i].node().apiType() != MFn::kDagPose)
				{
					continue;
				}

				MObject			aMember = connPlugs[i].attribute();
				MFnAttribute	fnAttr(aMember);

				auto name = fnAttr.name();
				if (name != "worldMatrix")
				{
					continue;
				}


				uint32_t jointIndex = connPlugs[i].logicalIndex();
				MFnDependencyNode nDagPose(connPlugs[i].node());

				// construct plugs for this joint's world matrix
				MObject aWorldMatrix = nDagPose.attribute("worldMatrix");
				MPlug	pWorldMatrix(connPlugs[i].node(), aWorldMatrix);

				pWorldMatrix.selectAncestorLogicalIndex(jointIndex, aWorldMatrix);

				// get the world matrix data
				MObject oWorldMatrix;
				status = pWorldMatrix.getValue(oWorldMatrix);
				if (MS::kSuccess != status) {
					// Problem retrieving world matrix
					MayaUtil::MayaPrintError("failed to get world matrix for bone(1): %s", bone.name.c_str());
					return status;
				}

				MFnMatrixData dMatrix(oWorldMatrix);
				MMatrix	m = dMatrix.matrix(&status);

				worldMatrix = MayaUtil::XMat(m);
				worldPos = MayaUtil::XVec(m);
				foundBindPose = true;
				break;
			}
		}
		else
		{
			MayaUtil::MayaPrintError("error getting bind pose for '%s' error: %s", bone.name.c_str(), status.errorString().asChar());
		}
	}

	if (!foundBindPose)
	{
		MayaUtil::MayaPrintVerbose("failed to get bind pose for bone: %s", bone.name.c_str());

		MTransformationMatrix worldTransMatrix = dagPath.inclusiveMatrix(&status);
		if (!status) {
			return status;
		}

		MMatrix m = worldTransMatrix.asMatrix();

		worldMatrix = MayaUtil::XMat(m);
		worldPos = MayaUtil::XVec(m);
	}


	// calculate scale and seperate it.
	Vec3f scale;
	scale.x = worldMatrix.getColumn(0).length();
	scale.y = worldMatrix.getColumn(1).length();
	scale.z = worldMatrix.getColumn(2).length();

	Matrix33f scaleMatrix = Matrix33f::createScale(scale);
	Matrix33f invScaleMatrix = scaleMatrix.inverted();
	Matrix33f rotationMatrix = invScaleMatrix * worldMatrix;

	bone.scale = scale;
	bone.bindpos = worldPos;
	bone.bindRotation = rotationMatrix;

	if(MayaUtil::IsVerbose())
	{
		Quatf quat = Quatf(bone.bindRotation);
		auto euler = quat.getEulerDegrees();

		MayaUtil::MayaPrintVerbose("Bone '%s' pos: (%g,%g,%g) ang: (%g,%g,%g) scale: (%g,%g,%g)",
			bone.name.c_str(),
			bone.bindpos.x, bone.bindpos.y, bone.bindpos.z,
			euler.x, euler.y, euler.z,
			scale.x, scale.y, scale.z
		);
	}

	return status;
}



ModelExporter::MeshNameStr ModelExporter::getMeshDisplayName(const MString& fullname)
{
	core::FixedStack<MeshNameStr, 16> Stack;
	core::StringTokenizer<char> tokens(fullname.asChar(), fullname.asChar() + fullname.length(), '|');
	core::StringRange<char> range(nullptr, nullptr);

	while (tokens.ExtractToken(range))
	{
		Stack.push(MeshNameStr(range.GetStart(), range.GetEnd()));
	}

	// ok the name is 2nd one.
	// check we have 2 tho.
	if (Stack.size() > 1) {
		Stack.pop();
	}

	return Stack.top();
}

bool ModelExporter::getMeshMaterial(MDagPath &dagPath, model::RawModel::Material& material)
{
	MDagPath path = dagPath;
	path.extendToShape();

	int32_t instanceNum = 0;
	if (path.isInstanced()) {
		instanceNum = path.instanceNumber();
	}

	MFnMesh fnMesh(path);
	MObjectArray sets;
	MObjectArray comps;
	MStatus status = fnMesh.getConnectedSetsAndMembers(instanceNum, sets, comps, true);
	if (!status) {
		MayaUtil::MayaPrintError("MFnMesh::getConnectedSetsAndMembers failed (%s)", status.errorString().asChar());
		return false;
	}

	for (int32_t i = 0; i < safe_static_cast<int32_t>(sets.length()); i++) {
		MObject set = sets[i];
		MObject comp = comps[i];

		MFnSet fnSet(set, &status);
		if (status == MS::kFailure) {
			MayaUtil::MayaPrintError("MFnSet constructor failed (%s)", status.errorString().asChar());
			continue;
		}

		// Make sure the set is a polygonal set.  If not, continue.
		MItMeshPolygon piter(path, comp, &status);
		if (status == MS::kFailure) {
			continue;
		}

		MObject shaderNode = findShader(set);
		if (shaderNode == MObject::kNullObj) {
			continue;
		}

		MFnDependencyNode Shader(shaderNode);

		MPlug colorPlug = Shader.findPlug("color", &status);
		if (status == MS::kFailure) {
			MayaUtil::MayaPrintError("material(%s) has no color channel", Shader.name().asChar());
			continue;
		}

		MPlug transparencyPlug = Shader.findPlug("transparency", &status);
		{
			MObject data;
			transparencyPlug.getValue(data);
			MFnNumericData val(data);
			val.getData(
				material.tansparency_[0], 
				material.tansparency_[1], 
				material.tansparency_[2]
			);	

			material.tansparency_[3] = 1.f;
		}

		MPlug specularColorPlug = Shader.findPlug("specularColor", &status);
		{
			MObject data;
			specularColorPlug.getValue(data);
			MFnNumericData val(data);
			val.getData(
				material.specCol_[0],
				material.specCol_[1],
				material.specCol_[2]
			);

			material.specCol_[3] = 1.f;
		}

		MPlug ambientColorPlug = Shader.findPlug("ambientColor", &status);
		{
			MObject data;
			ambientColorPlug.getValue(data);
			MFnNumericData val(data);
			val.getData(
				material.ambientColor_[0],
				material.ambientColor_[1],
				material.ambientColor_[2]
			);

			material.ambientColor_[3] = 1.f;
		}


		if (Shader.name().length() > 64) {
			MayaUtil::MayaPrintError("Material name too long MAX(64): '%s'", Shader.name().asChar());
			return false;
		}

		material.name_.assign(Shader.name().asChar());
		if (material.name_.isEmpty()) {
			MayaUtil::MayaPrintError("material name is empty");
			return false;
		}

		return true;
	}

	return false;
}

MObject ModelExporter::findShader(const MObject& setNode)
{
	MStatus				status;
	MFnDependencyNode	fnNode(setNode);
	MPlug				shaderPlug;

	shaderPlug = fnNode.findPlug("surfaceShader");
	if (!shaderPlug.isNull()) {
		MPlugArray connectedPlugs;
		bool asSrc = false;
		bool asDst = true;
		shaderPlug.connectedTo(connectedPlugs, asDst, asSrc, &status);

		if (connectedPlugs.length() != 1) {
			MayaUtil::MayaPrintError("FindShader: Error getting shader (%s)", status.errorString().asChar());
		}
		else {
			return connectedPlugs[0].node();
		}
	}

	return MObject::kNullObj;
}

// -----------------------------------------------

ModelExporterCmd::ModelExporterCmd()
{
}

ModelExporterCmd::~ModelExporterCmd()
{
}

MStatus ModelExporterCmd::doIt(const MArgList &args)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pJobSys);

	ModelExporter exportModel(gEnv->pJobSys, g_arena);

	return exportModel.convert(args);
}

bool ModelExporterCmd::hasSyntax(void) const
{
	return true;
}

MSyntax ModelExporterCmd::newSyntax(void)
{
	MSyntax syn;
	syn.enableEdit(false);
	syn.enableQuery(false);

	syn.addFlag("f", "file", MSyntax::kString);
	syn.addFlag("v", "verbose");
	syn.addFlag("m", "mode", MSyntax::kString);
	syn.addFlag("s", "scale", MSyntax::kDouble);

	syn.addFlag("fb", "force_bones", MSyntax::kBoolean);

	syn.addFlag("dir", "dir_path", MSyntax::kString);
	syn.addFlag("p", "progress", MSyntax::kString);

	syn.addFlag("l0", "LOD0", MSyntax::kString);
	syn.addFlag("l1", "LOD1", MSyntax::kString);
	syn.addFlag("l2", "LOD2", MSyntax::kString);
	syn.addFlag("l3", "LOD3", MSyntax::kString);

	return syn;
}

void* ModelExporterCmd::creator()
{
	return new ModelExporterCmd;
}
