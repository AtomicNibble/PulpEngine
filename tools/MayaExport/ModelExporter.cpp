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

MayaBone::MayaBone() : dagnode(nullptr)
{
	mayaNode.setOwner(this);
	exportNode.setOwner(this);

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

	mayaNode.setOwner(this);
	exportNode.setOwner(this);
	return *this;
}

// --------------------------------------------

ModelExporter::ModelExporter(core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena) :
	model::ModelCompiler(pJobSys, arena),
	mayaBones_(arena),
	exportMode_(ExpoMode::SERVER),
	meshExportMode_(MeshExpoMode::EXPORT_INPUT),
	unitOfMeasurement_(UnitOfMeasureMent::INCHES)
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
		bool unChanged = false;

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
					argsToJson(),
					compressed,
					&unChanged
				);

				if (!status) {
					X_ERROR("Model", "Failed update AssetDB");
					return status;
				}
			}
			{
				MayaUtil::SetProgressText("Compiling model");

				if (!unChanged)
				{
					PROFILE_MAYA_NAME("Compile and save");

					// we want the path from assetDB for the assets mod.
					// lower case the relative path add add to working.
					core::Path<char> assetPath;
					assetPath /= mod.outDir;
					assetPath /= assetDb::AssetType::ToString(maya::AssetDB::AssetType::MODEL);
					assetPath += "s";
					assetPath.toLower();
					assetPath /= getName();
					assetPath.replaceSeprators();

					core::Path<char> outPath = info.workingDir;
					outPath /= assetPath;

					MayaUtil::MayaPrintMsg("Exporting to: '%s'", outPath.c_str());

					if (!compileModel(outPath)) {
						MayaUtil::MayaPrintError("Failed to compile model");
						return MS::kFailure;
					}
				}
				else
				{
					MayaUtil::MayaPrintMsg("!Skiping compile asset unchanged");
				}
			}
		}
		else {
			MayaUtil::IncProcess();
		}

		saveOk = true;

		MayaUtil::SetProgressText("Complete");

		if (exportMode_ == ExpoMode::SERVER && !unChanged)
		{
			printStats();
		}
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
	CompileFlags::Description flagsDsc;

	info.append("\nModel Info:");
	info.appendFmt("\n> Compile Flags: %s", getFlags().ToString(flagsDsc));
	info.appendFmt("\n> Compile Time: %fms", stats_.compileTime.GetMilliSeconds());
	info.appendFmt("\n> Total Lods: %" PRIu32, stats_.totalLods);
	info.appendFmt("\n> Total Mesh: %" PRIu32, stats_.totalMesh);
	info.appendFmt("\n> Total Mesh merged: %" PRIu32, stats_.totalMeshMerged);
	info.appendFmt("\n> Total Col Mesh: %" PRIu32, stats_.totalColMesh);
	info.appendFmt("\n> Total Joints: %" PRIu32, stats_.totalJoints);
	info.appendFmt("\n> Total Joints dropped: %" PRIu32, stats_.totalJointsDropped);


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


	info.appendFmt("\n> Total Verts: %" PRIu32, stats_.totalVerts);
	info.appendFmt("\n> Total Verts Merged: %" PRIu32, stats_.totalVertsMerged);
	info.appendFmt("\n> Total Faces: %" PRIu32, stats_.totalFaces);
	info.appendFmt("\n> Total Weights dropped: %" PRIu32, stats_.totalWeightsDropped);
	info.append("\n");

	if (stats_.totalWeightsDropped > 0) {
		uint32_t maxWeights = model::MODEL_MAX_VERT_BINDS_NONE_EXT;
		if (getFlags().IsSet(model::ModelCompiler::CompileFlag::EXT_WEIGHTS)) {
			maxWeights = model::MODEL_MAX_VERT_BINDS;
		}

		info.appendFmt("!> bind weights where dropped, consider binding with max influences: %" PRIu32 "\n", maxWeights);
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

core::string ModelExporter::getName(void) const
{
	return name_;
}



MString ModelExporter::argsToJson(void) const
{
	core::json::StringBuffer s;
	core::json::Writer<core::json::StringBuffer> writer(s);

	writer.SetMaxDecimalPlaces(5);
	writer.StartObject();
	writer.Key("verbose");
	writer.Bool(MayaUtil::IsVerbose());
	writer.Key("mode");
	writer.String(ExpoMode::ToString(exportMode_));

	writer.Key("scale");
	writer.Double(this->getScale());
	writer.Key("weight_thresh");
	writer.Double(this->getJointWeightThreshold());
	writer.Key("uv_merge_thresh");
	writer.Double(this->getTexCoordElipson());
	writer.Key("vert_merge_thresh");
	writer.Double(this->getVertexElipson());
	writer.Key("vert_merge_thresh");
	writer.Double(this->getVertexElipson());

	const CompileFlags flags = getFlags();
	writer.Key("zero_origin");
	writer.Bool(flags.IsSet(CompileFlag::ZERO_ORIGIN));
	writer.Key("white_vert_col");
	writer.Bool(flags.IsSet(CompileFlag::WHITE_VERT_COL));
	writer.Key("merge_meshes");
	writer.Bool(flags.IsSet(CompileFlag::MERGE_MESH));
	writer.Key("merge_verts");
	writer.Bool(flags.IsSet(CompileFlag::MERGE_VERTS));
	writer.Key("ext_weights");
	writer.Bool(flags.IsSet(CompileFlag::EXT_WEIGHTS));

	// lods.
	for (size_t i = 0; i < model::MODEL_MAX_LODS; i++)
	{
		core::StackString<32> buf;
		buf.appendFmt("lod%" PRIuS "_dis", i);

		writer.Key(buf.c_str());

		if ((i+1) > lodExpoInfo_.size()) {
			writer.Double(0);
		}
		else {
			writer.Double(lodExpoInfo_[i].distance);
		}
	}

	writer.EndObject();

	return MString(s.GetString());
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
			MayaUtil::MayaPrintWarning("failed to get merge_meshes flag");
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

	idx = args.flagIndex("ext_weights");
	if (idx != MArgList::kInvalidArgIndex) {
		bool extWeights = false;
		if (!args.get(++idx, extWeights)) {
			MayaUtil::MayaPrintWarning("failed to get ext_weights flag");
		}
		else {
			if (extWeights) {
				flags.Set(CompileFlag::EXT_WEIGHTS);
			}
			else {
				flags.Remove(CompileFlag::EXT_WEIGHTS);
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
			for (int32_t x = 0; x < numVerts; x++) {
				model::RawModel::Vert& vert = mesh.verts_[x];
				vert.pos_ = MayaUtil::ConvertToGameSpace(MayaUtil::XVec(vertexArray[x]));
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

					GetLocalIndex(polygonVertices, vertexList, localIndex);

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
			MayaUtil::MayaPrintError("Joints: max bones reached: %" PRIu32, model::MODEL_MAX_BONES);
			return MStatus::kFailure;
		}

		if (new_bone.name.length() > model::MODEL_MAX_BONE_NAME_LENGTH)
		{
			MayaUtil::MayaPrintError("Joints: max pMayaBone name length exceeded, MAX: %" PRIu32 " '%s' -> %" PRIuS,
				model::MODEL_MAX_BONE_NAME_LENGTH, new_bone.name.c_str(), new_bone.name.length());
			return MStatus::kFailure;
		}

		size_t idx = mayaBones_.append(new_bone);

		MayaBone* pMayaBone = &mayaBones_[idx];
		pMayaBone->index = safe_static_cast<int, size_t>((mayaBones_.size() - 1));
	}


	// create hierarchy
	MayaBone* pMayaBone = mayaBones_.ptr();
	for (size_t i = 0; i < mayaBones_.size(); i++, pMayaBone++) 
	{
		if (!pMayaBone->dagnode) {
			continue;
		}

		pMayaBone->mayaNode.setParent(mayaHead_);
		pMayaBone->exportNode.setParent(exportHead_);

		auto parentNode = GetParentBone(pMayaBone->dagnode);
		if (parentNode) 
		{
			// do we have this joint?
			for (size_t j = 0; j < mayaBones_.size(); j++) {
				if (!mayaBones_[j].dagnode) {
					continue;
				}

				if (mayaBones_[j].dagnode->name() == parentNode->name()) {
					pMayaBone->mayaNode.setParent(mayaBones_[j].mayaNode);
					pMayaBone->exportNode.setParent(mayaBones_[j].exportNode);
					break;
				}
			}
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
		for (pMayaBone = exportHead_.next(); pMayaBone != nullptr; pMayaBone = pMayaBone->exportNode.next())
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


void ModelExporter::GetLocalIndex(MIntArray& getVertices, MIntArray& getTriangle, core::FixedArray<uint32_t, 8>& indexOut)
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

core::UniquePointer<MFnDagNode> ModelExporter::GetParentBone(MFnDagNode* pBone)
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


MStatus ModelExporter::getBindPose(const MObject &jointNode, MayaBone* pBone, float scale)
{
	X_ASSERT_NOT_NULL(pBone);

	MStatus				status;
	MFnDependencyNode	fnJoint(jointNode);
	MObject				aBindPose = fnJoint.attribute("bindPose", &status);

	pBone->bindpos = Vec3f::zero();
	pBone->bindm33 = Matrix33f::identity();

	bool set = false;

	if (MS::kSuccess == status)
	{
		MPlugArray	connPlugs;
		MPlug		pBindPose(jointNode, aBindPose);

		pBindPose.connectedTo(connPlugs, false, true);
		for (uint32_t i = 0; i < connPlugs.length(); ++i)
		{
			if (connPlugs[i].node().apiType() == MFn::kDagPose) 
			{
				MObject			aMember = connPlugs[i].attribute();
				MFnAttribute	fnAttr(aMember);

				if (fnAttr.name() == "worldMatrix")
				{
					uint32_t jointIndex = connPlugs[i].logicalIndex();

					MFnDependencyNode nDagPose(connPlugs[i].node());

					// construct plugs for this joint's world matrix
					MObject aWorldMatrix = nDagPose.attribute("worldMatrix");
					MPlug	pWorldMatrix(connPlugs[i].node(), aWorldMatrix);

					pWorldMatrix.selectAncestorLogicalIndex(jointIndex, aWorldMatrix);

					// get the world matrix data
					MObject worldMatrix;
					status = pWorldMatrix.getValue(worldMatrix);
					if (MS::kSuccess != status) {
						// Problem retrieving world matrix
						MayaUtil::MayaPrintError("failed to get world matrix for bone(1): %s", pBone->name.c_str());
						return status;
					}

					MFnMatrixData	dMatrix(worldMatrix);
					MMatrix			wMatrix = dMatrix.matrix(&status);

					pBone->bindm33 = MayaUtil::ConvertToGameSpace(MayaUtil::XMat(wMatrix));
					pBone->bindpos = MayaUtil::ConvertToGameSpace(MayaUtil::XVec(wMatrix)) * scale;

					set = true;
					break;
				}
			}
		}
	}
	else 
	{
		MayaUtil::MayaPrintError("error getting bind pose for '%s' error: %s", pBone->name.c_str(), status.errorString().asChar());
	}

	// failed to get the bind pose :(
	if (!set)
	{
		MayaUtil::MayaPrintVerbose("failed to get bind pose for bone: %s", pBone->name.c_str());

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

	syn.addFlag("wt", "weight_thresh", MSyntax::kDouble);
	syn.addFlag("umt", "uv_merge_thresh", MSyntax::kDouble);
	syn.addFlag("vmt", "vert_merge_thresh", MSyntax::kDouble);

	syn.addFlag("zo", "zero_origin", MSyntax::kBoolean);
	syn.addFlag("wvc", "white_vert_col", MSyntax::kBoolean);
	syn.addFlag("mm", "merge_meshes", MSyntax::kBoolean);
	syn.addFlag("mv", "merge_verts", MSyntax::kBoolean);
	syn.addFlag("ew", "ext_weights", MSyntax::kBoolean);
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
