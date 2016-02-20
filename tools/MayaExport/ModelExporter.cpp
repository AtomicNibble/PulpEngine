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
#include <maya/MTransformationMatrix.h>
#include <maya/MFnNumericData.h>


X_DISABLE_WARNING(4702)
#include <algorithm>
#include <map>
X_ENABLE_WARNING(4702)

#include <String\StringTokenizer.h>
#include <Containers\ByteStream.h>
#include <Containers\FixedStack.h>

#include <Threading\Thread.h>

#include "Profiler.h"
#include "MayaUtil.h"

X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\Foundation")
X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\OpenMaya")
X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\OpenMayaUI")
X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\OpenMayaAnim")

X_USING_NAMESPACE;


// --------------------------------------------

MayaBone::MayaBone() : dagnode(nullptr)
{
	mayaNode.setOwner(this);
	exportNode.setOwner(this);

	keep = false;
	bindpos = Vec3f::zero();
	bindm33 = Matrix33f::identity();
}

MayaBone::MayaBone(const MayaBone& oth) : 
mayaNode(oth.mayaNode), 
exportNode(oth.exportNode)
{
	name = oth.name;
	dagnode = oth.dagnode;
	index = oth.index;

	bindpos = oth.bindpos;
	bindm33 = oth.bindm33;

	keep = oth.keep;

	mayaNode.setOwner(this);
	exportNode.setOwner(this);
}

MayaBone& MayaBone::operator = (const MayaBone &oth)
{
	name = oth.name;
	dagnode = oth.dagnode;
	index = oth.index;

	bindpos = oth.bindpos;
	bindm33 = oth.bindm33;

	mayaNode = oth.mayaNode;
	exportNode = oth.exportNode;

	keep = oth.keep;

	mayaNode.setOwner(this);
	exportNode.setOwner(this);
	return *this;
}

// --------------------------------------------

ModelExporter::ModelExporter(core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena) :
	model::ModelCompiler(pJobSys, arena),
	mayaBones_(arena),
	exportMode_(ExpoMode::EXPORT_INPUT),
	unitOfMeasurement_(UnitOfMeasureMent::INCHES)
{
	tagOrigin_.index = 0;
	tagOrigin_.name.append("tag_origin");
	tagOrigin_.dagnode = nullptr;
	tagOrigin_.keep = false;

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
	uint i, x;
	MStatus status = MS::kSuccess;

	for (i = 0; i < safe_static_cast<uint32_t, size_t>(lodExpoInfo_.size()); i++)
	{
		MDagPathArray pathArry;
		MSelectionList list;
		LODExportInfo& info = lodExpoInfo_[i];
		
		MStringArray nameArr;
		info.objects.split(' ', nameArr);

		for (x = 0; x < nameArr.length(); x++) {
			list.add(nameArr[x]);
		}

		for (x = 0; x < list.length(); x++)
		{
			MDagPath path;
			status = list.getDagPath(x, path);
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

	//	int goat = pathArry.length();
	//	MayaPrintMsg("TEST: %s", info.objects.asChar());
	//	MayaPrintMsg("LOD%i object count: %i", i, pathArry.length());

		info.exportObjects = pathArry;
	}

	return MS::kSuccess;
}

MStatus ModelExporter::getExportObjects(void)
{
	if (exportMode_ == ExpoMode::EXPORT_INPUT){
		return getInputObjects();
	} else {
	//	if (g_options.exportMode_ == PotatoOptions::EXPORT_SELECTED)
	//		return getSelectedObjects();
	//	return getAllObjects();
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

	status = MayaUtil::ShowProgressDlg(0, 6);
	bool saveOk = false;

	{
	//	float appliedScale = scale_;
	//	float scale = g_options.scale_;

	//	if (g_options.unitOfMeasurement_ == PotatoOptions::INCHES) {
	//		scale = appliedScale * 2.54f;
	//	}

		MayaUtil::MayaPrintMsg("Exporting to: '%s'", getFilePath().c_str());
	//	MayaUtil::MayaPrintMsg("Scale: '%f' Applied: '%f'", scale, appliedScale);
		MayaUtil::MayaPrintMsg(""); // new line
	}


	// name length check
	if (fileName_.length() > model::MODEL_MAX_NAME_LENGTH)
	{
		MayaUtil::MayaPrintError("Model name is too long. MAX: %i, provided: %i",
			model::MODEL_MAX_NAME_LENGTH, fileName_.length());
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

		MayaUtil::SetProgressText("Saving raw model");

		// time to slap a goat!
		core::Path<char> outPath = getFilePath();

		{
			PROFILE_MAYA_NAME("Save Raw");

			if (!SaveRawModel(outPath)) {
				MayaUtil::MayaPrintError("Failed to save raw model");
				return MS::kFailure;
			}
		}

		MayaUtil::SetProgressText("Compiling model");

		{
			PROFILE_MAYA_NAME("Compile and save");

			if (!CompileModel(outPath)) {
				MayaUtil::MayaPrintError("Failed to compile model");
				return MS::kFailure;
			}
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

	info.append("\nModel Info:\n");
	info.appendFmt("> Compile Time: %fms", stats_.compileTime.GetMilliSeconds());
	info.appendFmt("\n> Total Lods: %i", stats_.totalLods);
	info.appendFmt("\n> Total Mesh: %i", stats_.totalMesh);
	info.appendFmt("\n> Total Mesh merged: %i", stats_.totalMeshMerged);
	info.appendFmt("\n> Total Joints: %i", stats_.totalJoints);
	info.appendFmt("\n> Total Joints dropped: %i", stats_.totalJointsDropped);


	if (stats_.droppedBoneNames.size() > 0) {
		info.append(" -> (");
		for (uint i = 0; i < stats_.droppedBoneNames.size(); i++) {
			info.append(stats_.droppedBoneNames[i].c_str());

			if (i < (stats_.droppedBoneNames.size() - 1)) {
				info.append(", ");
			}

			if (i > 9 && (i % 10) == 0) {
				info.append("\n");
			}
		}
		info.append(")");
	}
	if (stats_.droppedBoneNames.size() > 10) {
		info.append("\n");
	}


	info.appendFmt("\n> Total Verts: %i", stats_.totalVerts);
	info.appendFmt("\n> Total Faces: %i", stats_.totalFaces);
	info.appendFmt("\n> Total Weights dropped: %i", stats_.totalWeightsDropped);
	info.append("\n");

	if (stats_.totalWeightsDropped > 0) {
		info.append("!> bind weights where dropped, consider binding with max influences: 4\n");
	}

	{
		const AABB& b = stats_.bounds;
		const auto min = b.min;
		const auto max = b.max;
		info.append("> Bounds: ");
		info.appendFmt("(%g,%g,%g) <-> ", min[0], min[1], min[2]);
		info.appendFmt("(%g,%g,%g)\n", max[0], max[1], max[2]);

		const auto size = b.size();
		info.append("> Dimensions: ");
		info.appendFmt("w: %g d: %g h: %g", size[0], size[1], size[2]);

		if (unitOfMeasurement_ == UnitOfMeasureMent::INCHES) {
			info.append(" (inches)");
		}
		else {
			info.append(" (cm)");
		}
	}

	MayaUtil::MayaPrintMsg(info.c_str());
}

void ModelExporter::setFileName(const MString& path)
{
	core::StackString<512, char> temp(path.asChar());
	temp.trim();

	fileName_.set(temp.c_str());
	fileName_.setExtension(model::MODEL_FILE_EXTENSION);
}

void ModelExporter::setOutdir(const MString& path)
{
	core::StackString<512, char> temp(path.asChar());
	temp.trim();

	outDir_.set(temp.c_str());
//	outDir_.ensureSlash();
	outDir_.replaceSeprators();
}

core::Path<char> ModelExporter::getFilePath(void) const
{
	return outDir_ / fileName_;
}

MStatus ModelExporter::parseArgs(const MArgList& args)
{
	MStatus status;
	uint32_t idx;

	float scale = 1.f;
	CompileFlags flags = (CompileFlag::MERGE_MESH | CompileFlag::MERGE_VERTS | CompileFlag::WHITE_VERT_COL);

	idx = args.flagIndex("f");
	if (idx == MArgList::kInvalidArgIndex) {
		MayaUtil::MayaPrintError("missing file argument");
		return MS::kFailure;
	}

	// required
	{
		MString Mfilename;

		status = args.get(++idx, Mfilename);
		if (!status) {
			MayaUtil::MayaPrintError("missing filename");
			return status;
		}

		setFileName(Mfilename);
	}

	// allow for options to be parsed.
	idx = args.flagIndex("verbose");
	if (idx != MArgList::kInvalidArgIndex) {
		MayaUtil::SetVerbose(true);
	}

	idx = args.flagIndex("scale");
	if (idx != MArgList::kInvalidArgIndex) {
		double temp;
		if (!args.get(++idx, temp)) {
			MayaUtil::MayaPrintWarning("failed to get scale flag");
		}
		else {
			scale = static_cast<float>(temp);
		}
	}

	idx = args.flagIndex("use_cm");
	if (idx != MArgList::kInvalidArgIndex) {
		bool useCm = false;
		if (!args.get(++idx, useCm)) {
			MayaUtil::MayaPrintWarning("failed to get use_cm flag");
		}
		else {
			if (useCm) {
				unitOfMeasurement_ = UnitOfMeasureMent::CM;
			}
		}
	}

	idx = args.flagIndex("weight_thresh");
	if (idx != MArgList::kInvalidArgIndex) {
		double temp;
		if (!args.get(++idx, temp)) {
			MayaUtil::MayaPrintWarning("failed to get weight_thresh flag");
		}
		else {
			SetJointWeightThreshold(static_cast<float>(temp));
		}
	}


	idx = args.flagIndex("uv_merge_thresh");
	if (idx != MArgList::kInvalidArgIndex) {
		double temp;
		if (!args.get(++idx, temp)) {
			MayaUtil::MayaPrintWarning("failed to get uv_merge_thresh flag");
		}
		else {
			SetTexCoordElipson(static_cast<float>(temp));
		}
	}

	idx = args.flagIndex("vert_merge_thresh");
	if (idx != MArgList::kInvalidArgIndex) {
		double temp;
		if (!args.get(++idx, temp)) {
			MayaUtil::MayaPrintWarning("failed to get vert_merge_thresh flag");
		}
		else {
			SetVertexElipson(static_cast<float>(temp));
		}
	}


	idx = args.flagIndex("zero_origin");
	if (idx != MArgList::kInvalidArgIndex) {
		bool zeroOrigin = false;
		if (!args.get(++idx, zeroOrigin)) {
			MayaUtil::MayaPrintWarning("failed to get zero_origin flag");
		}
		else {
			if (zeroOrigin) {
				flags.Set(CompileFlag::ZERO_ORIGIN);
			}
			else {
				flags.Remove(CompileFlag::ZERO_ORIGIN);
			}
		}
	}

	idx = args.flagIndex("white_vert_col");
	if (idx != MArgList::kInvalidArgIndex) {
		bool whiteVert = false;
		if (!args.get(++idx, whiteVert)) {
			MayaUtil::MayaPrintWarning("failed to get white_vert_col flag");
		}
		else {
			if (whiteVert) {
				flags.Set(CompileFlag::WHITE_VERT_COL);
			}
			else {
				flags.Remove(CompileFlag::WHITE_VERT_COL);
			}
		}
	}

	idx = args.flagIndex("merge_meshes");
	if (idx != MArgList::kInvalidArgIndex) {
		bool mergeMesh = false;
		if (!args.get(++idx, mergeMesh)) {
			MayaUtil::MayaPrintWarning("failed to get white_vert_col flag");
		}
		else {
			if (mergeMesh) {
				flags.Set(CompileFlag::MERGE_MESH);
			}
			else {
				flags.Remove(CompileFlag::MERGE_MESH);
			}
		}
	}

	idx = args.flagIndex("merge_verts");
	if (idx != MArgList::kInvalidArgIndex) {
		bool mergeMesh = false;
		if (!args.get(++idx, mergeMesh)) {
			MayaUtil::MayaPrintWarning("failed to get merge_verts flag");
		}
		else {
			if (mergeMesh) {
				flags.Set(CompileFlag::MERGE_VERTS);
			}
			else {
				flags.Remove(CompileFlag::MERGE_VERTS);
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
			lodName.appendFmt("LOD%i", i);
			idx = args.flagIndex(lodName.c_str());
			if (idx != MArgList::kInvalidArgIndex)
			{
				double temp;
				LODExportInfo info;
				info.idx = currentLod;
				if (!args.get(++idx, temp) ||
					!args.get(++idx, info.objects)) {
					MayaUtil::MayaPrintError("failed to parse LOD%i info", currentLod);
					continue;
				}
				if (lodExpoInfo_.size() == model::MODEL_MAX_LODS) {
					MayaUtil::MayaPrintError("Exceeded max load count: %i", model::MODEL_MAX_LODS);
					return MS::kFailure;
				}

				info.distance = static_cast<float>(temp);
				lodExpoInfo_.append(info);

				currentLod++;
			}
		}

		//	MayaPrintMsg("Total LOD: %i", currentLod);
		if (currentLod == 0) {
			MayaUtil::MayaPrintError("Failed to parse and LOD info");
			return MS::kFailure;
		}
	}

	// use the scale to make it cm -> inches.
	// this applyies it post user scale.
	if (unitOfMeasurement_ == UnitOfMeasureMent::INCHES) {
		scale = scale * 0.393700787f;
	}

	// set values.
	if (scale != 1.f) {
		SetScale(scale);
	}

	setFlags(flags);
	return status;
}


MStatus ModelExporter::loadLODs(void)
{
	PROFILE_MAYA("load LODS");
	MStatus status;

	lods_.resize(lodExpoInfo_.size(), model::RawModel::Lod(g_arena));

	for (size_t i = 0; i < lodExpoInfo_.size(); i++)
	{
		LODExportInfo& info = lodExpoInfo_[i];
		uint32_t numMesh = info.exportObjects.length();

		if (numMesh == 0) {
			MayaUtil::MayaPrintError("LOD%i failed to load meshes for lod no meshes found.", i);
			return MS::kFailure;
		}
		
		model::RawModel::Lod& lod = lods_[i];

		// set lod distance.
		lod.distance_ = info.distance;

		// reserver the meshes.
		lod.meshes_.resize(numMesh, model::RawModel::Mesh(g_arena));

	//	float scale = g_options.scale_;
	//	float jointThreshold = g_options.jointThreshold_;

		MFloatPointArray	vertexArray;
		MFloatVectorArray	normalsArray, tangentsArray, binormalsArray;
		MStringArray		UVSets;
		MFloatArray			u, v;
		MPointArray			points;
		MIntArray			polygonVertices;
		MIntArray			vertexList;
		MColorArray			vertColorsArray;

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

			if (!getMeshMaterial(info.exportObjects[meshIdx], material)) {
				MayaUtil::MayaPrintError("Mesh(%s): failed to get material", fnmesh.name().asChar());
				return MS::kFailure;
			}

			int numVerts = fnmesh.numVertices(&status);
			int numPoly = fnmesh.numPolygons(&status);

			if (fnmesh.numUVSets() < 1) {
				MayaUtil::MayaPrintError("Mesh(%s): has no uv sets: %s", fnmesh.name().asChar());
				return MS::kFailure;
			}
			//		int numNormals = fnmesh.numNormals(&status);

			if (!status) {
				MayaUtil::MayaPrintError("Mesh(%s): failed to get mesh info (%s)", fnmesh.name().asChar(), status.errorString().asChar());
				return status;
			}

			// resize baby.
			mesh.verts_.resize(numVerts);
			mesh.face_.setGranularity(numPoly * 3);
		//	mesh->weights.setGranularity(2048 * 2);

			status = fnmesh.getUVSetNames(UVSets);
			if (!status) {
				MayaUtil::MayaPrintError("Mesh(%s): failed to get UV set names (%s)",
					fnmesh.name().asChar(), status.errorString().asChar());
				return status;
			}

			// print how many :Z
			MayaUtil::MayaPrintVerbose("NumUvSets: %i", UVSets.length());
			for (uint32_t x = 0; x < UVSets.length(); x++) {
				MayaUtil::MayaPrintVerbose("-> Set(%i): %s", x, UVSets[static_cast<uint32_t>(i)].asChar());
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


			{
				using std::cerr;
				using std::endl;
				CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getUVs(u, v, &uvSet));
				CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getPoints(vertexArray, MSpace::kWorld));
				CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getNormals(normalsArray, MSpace::kWorld));
				CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getTangents(tangentsArray, MSpace::kWorld));
				CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getBinormals(binormalsArray, MSpace::kWorld));

				vertColorsArray.setSizeIncrement(numVerts);

				if (fnmesh.numColorSets() > 0) {
					CHECK_MSTATUS_AND_RETURN_IT(fnmesh.getVertexColors(vertColorsArray));
				}
			}

			MayaUtil::MayaPrintVerbose("u: %i v: %i", u.length(), v.length());
			MayaUtil::MayaPrintVerbose("NumVerts: %i", numVerts);
			MayaUtil::MayaPrintVerbose("NumPoly: %i", numPoly);

			// some times we don't have vert colors it seams so.
			// fill with white.
			MColor white;
			while (safe_static_cast<int, size_t>(vertColorsArray.length()) < numVerts)
			{
				vertColorsArray.append(white);
			}

			// verts
			int32_t x;
			for (x = 0; x < numVerts; x++) {
				model::RawModel::Vert& vert = mesh.verts_[x];
				vert.pos_ = MayaUtil::ConvertToGameSpace(MayaUtil::XVec(vertexArray[x]));
				vert.normal_ = MayaUtil::XVec(normalsArray[x]);
				vert.tangent_ = MayaUtil::XVec(tangentsArray[x]);
				vert.biNormal_ = MayaUtil::XVec(binormalsArray[x]);
				vert.uv_ = Vec2f(u[x], v[x]);
				vert.col_ = MayaUtil::XVec(vertColorsArray[x]);
			}

			// load the Faces and UV's
			MItMeshPolygon itPolygon(info.exportObjects[meshIdx], MObject::kNullObj);
			for (; !itPolygon.isDone(); itPolygon.next())
			{
				int numTriangles;

				itPolygon.numTriangles(numTriangles);
				itPolygon.getVertices(polygonVertices);

				while (numTriangles--)
				{
					status = itPolygon.getTriangle(numTriangles, points, vertexList, MSpace::kWorld);

					mesh.face_.append(model::RawModel::Face(vertexList[0], vertexList[1], vertexList[2]));
				}
			}



		}


	}

	return status;
}

MStatus ModelExporter::loadBones(void)
{
	PROFILE_MAYA("load joints");

	MStatus			status;
	MDagPath		dagPath;
	MFnDagNode		*parentNode;
	MayaBone		*pMayaBone;
	uint				i, j;

	float scale = 1.f; // g_options.scale_;


	// allocate max.
	mayaBones_.reserve(model::MODEL_MAX_BONES);

	MItDag dagIterator(MItDag::kDepthFirst, MFn::kTransform, &status);
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
		new_bone.dagnode = X_NEW(MFnDagNode, g_arena, "BoneDagNode")(dagPath, &status);

		if (!status) {
			X_DELETE(new_bone.dagnode, g_arena);// dunno if maya returns null, don't matter tho.
			MayaUtil::MayaPrintError("Joint: MFnDagNode constructor failed (%s)", status.errorString().asChar());
			return status;
		}

		new_bone.name.append(new_bone.dagnode->name().asChar());

		if (mayaBones_.size() == model::MODEL_MAX_BONES) {
			MayaUtil::MayaPrintError("Joints: max bones reached: %i", model::MODEL_MAX_BONES);
			return MStatus::kFailure;
		}

		if (new_bone.name.length() > model::MODEL_MAX_BONE_NAME_LENGTH)
		{
			MayaUtil::MayaPrintError("Joints: max pMayaBone name length exceeded, MAX: %i '%' -> %i",
				model::MODEL_MAX_BONE_NAME_LENGTH, new_bone.name.c_str(), new_bone.name.length());
			return MStatus::kFailure;
		}

		size_t idx = mayaBones_.append(new_bone);

		pMayaBone = &mayaBones_[idx];
		pMayaBone->index = safe_static_cast<int, size_t>((mayaBones_.size() - 1));
	}


	// create hierarchy
	pMayaBone = mayaBones_.ptr();
	for (i = 0; i < mayaBones_.size(); i++, pMayaBone++) {
		if (!pMayaBone->dagnode) {
			continue;
		}

		pMayaBone->mayaNode.setParent(mayaHead_);
		pMayaBone->exportNode.setParent(exportHead_);

		parentNode = GetParentBone(pMayaBone->dagnode);
		if (parentNode) {

			// do we have this joint?
			for (j = 0; j < mayaBones_.size(); j++) {
				if (!mayaBones_[j].dagnode) {
					continue;
				}

				if (mayaBones_[j].dagnode->name() == parentNode->name()) {
					pMayaBone->mayaNode.setParent(mayaBones_[j].mayaNode);
					pMayaBone->exportNode.setParent(mayaBones_[j].exportNode);
					break;
				}
			}
			X_DELETE(parentNode, g_arena);
		}

		pMayaBone->dagnode->getPath(dagPath);
		status = getBindPose(dagPath.node(&status), pMayaBone, scale);
		if (!status) {
			return status;
		}
	}

	// update idx's
	uint32_t index = 0;
	for (pMayaBone = exportHead_.next(); pMayaBone != nullptr; pMayaBone = pMayaBone->exportNode.next()) {
		pMayaBone->index = index++;
	}

	// fill in the raw bones.
	{
		bones_.reserve(mayaBones_.size());

		pMayaBone = nullptr;
		for (pMayaBone = exportHead_.next(); pMayaBone != nullptr;
				pMayaBone = pMayaBone->exportNode.next())
		{
			model::RawModel::Bone bone;
			bone.name_ = pMayaBone->name.c_str();
			bone.rotation_ = pMayaBone->bindm33;
			bone.worldPos_ = pMayaBone->bindpos;
			
			MayaBone* pParent = pMayaBone->exportNode.parent();

			if (pParent)
			{
				bone.parIndx_ = pParent->index;
			}
			else
			{
				bone.parIndx_ = -1;
			}

			bones_.append(bone);
		}
	}

	return status;
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


MFnDagNode* ModelExporter::GetParentBone(MFnDagNode* pBone)
{
	X_ASSERT_NOT_NULL(pBone);

	MStatus		status;
	MObject		parentObject;

	parentObject = pBone->parent(0, &status);
	if (!status && status.statusCode() == MStatus::kInvalidParameter) {
		return NULL;
	}

	while (!parentObject.hasFn(MFn::kTransform)) {
		MFnDagNode parentNode(parentObject, &status);
		if (!status) {
			return NULL;
		}

		parentObject = parentNode.parent(0, &status);
		if (!status && status.statusCode() == MStatus::kInvalidParameter) {
			return NULL;
		}
	}

	MFnDagNode *parentNode;

	parentNode = X_NEW(MFnDagNode, g_arena, "ParentDagNode")(parentObject, &status);
	if (!status) {
		X_DELETE(parentNode, g_arena);
		return NULL;
	}

	return parentNode;
}


MStatus ModelExporter::getBindPose(const MObject &jointNode, MayaBone* pBone, float scale)
{
	X_ASSERT_NOT_NULL(pBone);

	MStatus				status;
	MFnDependencyNode	fnJoint(jointNode);
	MObject				aBindPose = fnJoint.attribute("bindPose", &status);

	pBone->bindpos = Vec3f::zero();
	pBone->bindm33 = Matrix33f::identity();

	bool set = false;

	if (MS::kSuccess == status) {
		unsigned	ii;
		unsigned	jointIndex;
		unsigned	connLength;
		MPlugArray	connPlugs;
		MPlug		pBindPose(jointNode, aBindPose);

		pBindPose.connectedTo(connPlugs, false, true);
		connLength = connPlugs.length();
		for (ii = 0; ii < connLength; ++ii) {
			if (connPlugs[ii].node().apiType() == MFn::kDagPose) {
				MObject			aMember = connPlugs[ii].attribute();
				MFnAttribute	fnAttr(aMember);

				if (fnAttr.name() == "worldMatrix") {
					jointIndex = connPlugs[ii].logicalIndex();

					MFnDependencyNode nDagPose(connPlugs[ii].node());

					// construct plugs for this joint's world matrix
					MObject aWorldMatrix = nDagPose.attribute("worldMatrix");
					MPlug	pWorldMatrix(connPlugs[ii].node(), aWorldMatrix);

					pWorldMatrix.selectAncestorLogicalIndex(jointIndex, aWorldMatrix);

					// get the world matrix data
					MObject worldMatrix;
					status = pWorldMatrix.getValue(worldMatrix);
					if (MS::kSuccess != status) {
						// Problem retrieving world matrix
						MayaUtil::MayaPrintError("failed to get world matrix for bone(1): %s",
							pBone->name.c_str());
						return status;
					}

					MFnMatrixData	dMatrix(worldMatrix);
					MMatrix			wMatrix = dMatrix.matrix(&status);

					pBone->bindm33 = MayaUtil::ConvertToGameSpace(MayaUtil::XMat(wMatrix));
					pBone->bindpos = MayaUtil::ConvertToGameSpace(MayaUtil::XVec(wMatrix)) * scale;

					//if (!options.ignoreScale) {
					//	joint->bindpos *= joint->scale;
					//}
					set = true;
					break;
				}
			}
		}
	}
	else {
		MayaUtil::MayaPrintError("error getting bind pose for '%s' error: %s",
			pBone->name.c_str(), status.errorString().asChar());
	}

	// failed to get the bind pose :(
	if (!set)
	{
		MayaUtil::MayaPrintVerbose("failed to get bind pose for bone: %s",
			pBone->name.c_str());

		// try get world pos.

		MDagPath path;
		status = pBone->dagnode->getPath(path);
		if (status != MS::kSuccess) {
			MayaUtil::MayaPrintError("Failed to get bone path: '%s'", pBone->name.c_str());
			return MS::kFailure;
		}

		MTransformationMatrix worldMatrix = path.inclusiveMatrix();
		MMatrix m = worldMatrix.asMatrix();
		pBone->bindm33 = MayaUtil::ConvertToGameSpace(MayaUtil::XMat(m));
		pBone->bindpos = MayaUtil::ConvertToGameSpace(MayaUtil::XVec(m)) * scale;
	}

	MayaUtil::MayaPrintVerbose("Bone '%s' pos: (%g,%g,%g)",
		pBone->name.c_str(), pBone->bindpos.x, pBone->bindpos.y, pBone->bindpos.z);

	return status;
}



core::StackString<60> ModelExporter::getMeshDisplayName(const MString& fullname)
{
	typedef core::StackString<60> NameType;
	core::FixedStack<NameType, 16> Stack;

	core::StringTokenizer<char> tokens(fullname.asChar(), fullname.asChar() + fullname.length(), '|');
	core::StringRange<char> range(nullptr, nullptr);

	while (tokens.ExtractToken(range))
	{
		Stack.push(NameType(range.GetStart(), range.GetEnd()));
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
	MStatus	 status;
	MDagPath path = dagPath;
	int	i;
	int	instanceNum;

	path.extendToShape();

	instanceNum = 0;
	if (path.isInstanced()) {
		instanceNum = path.instanceNumber();
	}

	MFnMesh fnMesh(path);
	MObjectArray sets;
	MObjectArray comps;
	status = fnMesh.getConnectedSetsAndMembers(instanceNum, sets, comps, true);
	if (!status) {
		MayaUtil::MayaPrintError("MFnMesh::getConnectedSetsAndMembers failed (%s)", status.errorString().asChar());
	}

	for (i = 0; i < (int)sets.length(); i++) {
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

		MObject shaderNode = FindShader(set);
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
			val.getData(material.tansparency_[0], 
				material.tansparency_[1], 
				material.tansparency_[2]);	

			material.tansparency_[3] = 1.f;
		}

		MPlug specularColorPlug = Shader.findPlug("specularColor", &status);
		{
			MObject data;
			specularColorPlug.getValue(data);
			MFnNumericData val(data);
			val.getData(material.specCol_[0],
				material.specCol_[1],
				material.specCol_[2]);

			material.specCol_[3] = 1.f;
		}

		MPlug ambientColorPlug = Shader.findPlug("ambientColor", &status);
		{
			MObject data;
			ambientColorPlug.getValue(data);
			MFnNumericData val(data);
			val.getData(material.ambientColor_[0],
				material.ambientColor_[1],
				material.ambientColor_[2]);

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

MObject ModelExporter::FindShader(MObject& setNode)
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

void* ModelExporterCmd::creator()
{
	return new ModelExporterCmd;
}
