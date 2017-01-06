#include "stdafx.h"
#include "ModelCompiler.h"

#include <Threading\JobSystem2.h>
#include <Time\StopWatch.h>
#include <Util\Cpu.h>

#include <IModel.h>
#include <IMaterial.h>
#include <IFileSys.h>


#include "FaceOptermize.h"

X_DISABLE_WARNING(4702)
#include <algorithm>
#include <map>
#include <numeric>
X_ENABLE_WARNING(4702)

X_NAMESPACE_BEGIN(model)

namespace
{
	uint32_t hashsingle(float f)
	{
		union {
			unsigned int ui;
			float fv;
		};
		fv = f;
		return ((ui & 0xfffff000) >> 12);
	}

	int64 Hash(float x, float y, float z)
	{
		int64 h1 = hashsingle(x);
		int64 h2 = hashsingle(y);
		int64 h3 = hashsingle(z);
		return (h1 << 40) | (h2 << 20) | h3;
	}

	int64 vertHash(const ModelCompiler::Vert& vert)
	{
		return Hash(vert.pos_[0], vert.pos_[1], vert.pos_[2]);
	}


	struct Hashcontainer
	{
		Hashcontainer() {
			pVert = nullptr;
			idx = 0;
		}

		ModelCompiler::Vert* pVert;
		int32_t idx;
	};

} // namesace 

const float ModelCompiler::MERGE_VERTEX_ELIPSION = 0.05f;
const float ModelCompiler::MERGE_TEXCOORDS_ELIPSION = 0.02f;
const float ModelCompiler::JOINT_WEIGHT_THRESHOLD = 0.005f;
const int32_t ModelCompiler::VERTEX_MAX_WEIGHTS = model::MODEL_MAX_VERT_BINDS;
const ModelCompiler::CompileFlags ModelCompiler::DEFAULT_FLAGS = ModelCompiler::CompileFlag::WHITE_VERT_COL |
	ModelCompiler::CompileFlag::MERGE_MESH;



ModelCompiler::Stats::Stats(core::MemoryArenaBase* arena) :
	droppedBoneNames(arena)
{
	clear();
}

void ModelCompiler::Stats::print(CompileFlags flags) const
{
	X_LOG0("Model", "Model Info:");
	X_LOG0("Model", "> Compile Time: %fms", compileTime.GetMilliSeconds());
	X_LOG0("Model", "> Total Lods: %" PRIu32, totalLods);
	X_LOG0("Model", "> Total Mesh: %" PRIu32, totalMesh);
	X_LOG0("Model", "> Total Mesh Merged: %" PRIu32, totalMeshMerged);
	X_LOG0("Model", "> Total Col Mesh: %" PRIu32, totalColMesh);
	X_LOG0("Model", "> Total Joints: %" PRIu32, totalJoints);
	X_LOG0("Model", "> Total Joints Dropped: %" PRIu32, totalJointsDropped);

	core::StackString<1024> info;

	if (droppedBoneNames.size() > 0) {
		info.append(" -> (");
		for (uint i = 0; i < droppedBoneNames.size(); i++) {
			info.append(droppedBoneNames[i].c_str());

			if (i < (droppedBoneNames.size() - 1)) {
				info.append(", ");
			}

			if (i > 9 && (i % 10) == 0) {
				info.append("");
			}
		}
		info.append(")");
	}
	if (droppedBoneNames.size() > 10) {
		info.append("");
	}

	X_LOG0("Model", info.c_str());
	X_LOG0("Model", "> Total Verts: %" PRIu32, totalVerts);
	X_LOG0("Model", "> Total Verts Merged: %" PRIu32, totalVertsMerged);
	X_LOG0("Model", "> Total Faces: %" PRIu32, totalFaces);
	X_LOG0("Model", "> Total eights Dropped: %" PRIu32, totalWeightsDropped);

	if (totalWeightsDropped > 0) {
		uint32_t maxWeights = model::MODEL_MAX_VERT_BINDS_NONE_EXT;
		if (flags.IsSet(model::ModelCompiler::CompileFlag::EXT_WEIGHTS)) {
			maxWeights = model::MODEL_MAX_VERT_BINDS;
		}

		X_LOG0("Model", "!> bind weights where dropped, consider binding with max influences: %" PRIu32, maxWeights);
	}

	{
		const AABB& b = bounds;
		const auto min = b.min;
		const auto max = b.max;
		info.set("> Bounds: ");
		info.appendFmt("(%g,%g,%g) <-> ", min[0], min[1], min[2]);
		info.appendFmt("(%g,%g,%g)", max[0], max[1], max[2]);
		X_LOG0("Model", info.c_str());

		const auto size = b.size();
		info.set("> Dimensions: ");
		info.appendFmt("w: %g d: %g h: %g", size[0], size[1], size[2]);
		X_LOG0("Model", info.c_str());
	}
}


void ModelCompiler::Stats::clear(void)
{
	totalLods = 0;
	totalMesh = 0;
	totalColMesh = 0;
	totalJoints = 0;
	totalJointsDropped = 0;
	totalVerts = 0;
	totalVertsMerged = 0;
	totalFaces = 0;
	totalWeightsDropped = 0;
	totalMeshMerged = 0;

	droppedBoneNames.clear();
	droppedBoneNames.setGranularity(16);

	bounds.clear();
}


// ---------------------------------------------------------------


ModelCompiler::Binds::Binds(core::MemoryArenaBase* arena) :
	simple_(arena),
	stream_(arena)
{
}


bool ModelCompiler::Binds::write(core::XFileScoped& file)
{
	if (simple_.isNotEmpty())
	{
		return file.writeObjs(simple_.ptr(), simple_.size()) == simple_.size();
	}
	else if(stream_.size() > 0)
	{
		return file.write(stream_.begin(), stream_.size()) == stream_.size();
	}

	// no binds.
	return true;
}


void ModelCompiler::Binds::populate(const VertsArr& verts)
{
	CompbindInfo::BindCountsArr bindCounts{};

	if (verts.isEmpty()) {
		return;
	}

	// work out the bind counts
	for (const auto& vert : verts) {

		const size_t numBinds = vert.binds_.size();
		if (numBinds > 0) { // fucking branch, maybe I should force binds.
			bindCounts[numBinds - 1]++;
		}
	}

	bindInfo_.set(bindCounts);

	// work out the size of the stream
	size_t streamSize = bindInfo_.dataSizeTotal();

	stream_.free();
	stream_.resize(streamSize);

	// simple binds?
	if (bindCounts[0] == verts.size())
	{
		int32_t curBoneIdx = verts[0].binds_[0].boneIdx_; // safe
		size_t i, lastVertIdx = 0;

		// setup first vert.
		simpleBind sb;
		sb.jointIdx = safe_static_cast<uint16_t,int32_t>(curBoneIdx);
		sb.faceOffset = 0;
		sb.numFaces = 0;
		sb.numVerts = 0;

		for (i = 1; i < verts.size(); i++)
		{
			const Vert& vert = verts[i];
			const RawModel::Bind& bind = vert.binds_[0];

			if (bind.boneIdx_ != curBoneIdx)
			{
				size_t numVerts = i - lastVertIdx;
				sb.faceOffset = 0;
				sb.numFaces = 0;
				sb.numVerts = safe_static_cast<uint16_t, size_t>(numVerts);

				simple_.append(sb);

				lastVertIdx = i;
				sb.jointIdx = bind.boneIdx_;
			}
		}

		// trailing?
		if (lastVertIdx != (verts.size() - 1))
		{
			size_t numVerts = i - lastVertIdx;

			sb.faceOffset = 0;
			sb.numFaces = 0;
			sb.numVerts = safe_static_cast<uint16_t, size_t>(numVerts);
			simple_.append(sb);
		}
	}
	else
	{
		for (const auto& vert : verts)
		{
			for (size_t i = 0; i < vert.binds_.size(); i++)
			{
				const RawModel::Bind& bind = vert.binds_[i];

				bindBone boneIdx(bind.boneIdx_);
				bindWeight weight(bind.weight_);

				stream_.write(boneIdx);

				// only write weight if idx > 0
				if (i > 0) {
					stream_.write(weight);
				}
			}
		}
	}
}

const size_t ModelCompiler::Binds::numSimpleBinds(void) const
{
	return simple_.size();
}

const CompbindInfo::BindCountsArr& ModelCompiler::Binds::getBindCounts(void) const
{
	return bindInfo_.getBindCounts();
}

const size_t ModelCompiler::Binds::dataSizeTotal(void) const
{
	return bindInfo_.dataSizeTotal();
}


// ---------------------------------------------------------------

ModelCompiler::Mesh::Mesh(const Mesh& mesh) :
	arena_(mesh.arena_),
	name_(mesh.name_),
	displayName_(mesh.displayName_),
	verts_(mesh.verts_),
	faces_(mesh.faces_),
	binds_(mesh.binds_),
	colMeshes_(mesh.colMeshes_),
	material_(mesh.material_),
	boundingBox_(mesh.boundingBox_)
{

}

ModelCompiler::Mesh::Mesh(Mesh&& mesh) :
	arena_(std::move(mesh.arena_)),
	name_(std::move(mesh.name_)),
	displayName_(std::move(mesh.displayName_)),
	verts_(std::move(mesh.verts_)),
	faces_(std::move(mesh.faces_)),
	binds_(std::move(mesh.binds_)),
	colMeshes_(std::move(mesh.colMeshes_)),
	material_(std::move(mesh.material_)),
	boundingBox_(std::move(mesh.boundingBox_))
{

}

ModelCompiler::Mesh::Mesh(core::MemoryArenaBase* arena) :
	arena_(arena),
	verts_(arena),
	faces_(arena),
	binds_(arena),
	colMeshes_(arena)
{

}

void ModelCompiler::Mesh::calBoundingbox(void)
{
	AABB aabb;

	aabb.clear();

	for (const auto& vert : verts_) {
		aabb.add(vert.pos_);
	}

	boundingBox_ = aabb;
}

// ---------------------------------------------------------------

//ModelCompiler::ColMesh::ColMesh(core::MemoryArenaBase* arena) :
//	Mesh(arena),
//	type_(ColMeshType::CONVEX)
//{
//
//}

ModelCompiler::ColMesh::ColMesh(const ColMesh& oth) :
	Mesh(oth),
	type_(oth.type_),
	sphere_(oth.sphere_)
{

}



ModelCompiler::ColMesh::ColMesh(const Mesh& oth, ColMeshType::Enum type) :
	Mesh(oth.arena_),
	type_(type)
{

}

ColMeshType::Enum ModelCompiler::ColMesh::getType(void) const
{
	return type_;
}

// ---------------------------------------------------------------


ModelCompiler::Lod::Lod(core::MemoryArenaBase* arena) :
	distance_(0.f),
	meshes_(arena)
{

}

size_t ModelCompiler::Lod::getSubDataSize(const Flags8<model::StreamType>& streams) const
{
	size_t size = 0;

	for (auto& mesh : meshes_)
	{

		size += sizeof(model::Face) * mesh.faces_.size();
		size += sizeof(model::Vertex) * mesh.verts_.size();

		// streams.
		if (streams.IsSet(model::StreamType::COLOR)) {
			size += sizeof(model::VertexColor) * mesh.verts_.size();
		}
		if (streams.IsSet(model::StreamType::NORMALS)) {
			size += sizeof(model::VertexNormal) * mesh.verts_.size();
		}
		if (streams.IsSet(model::StreamType::TANGENT_BI)) {
			size += sizeof(model::VertexTangentBi) * mesh.verts_.size();
		}

		// bind data
		size += safe_static_cast<size_t, size_t>(mesh.binds_.dataSizeTotal());
	}

	return size;
}

size_t ModelCompiler::Lod::numMeshes(void) const
{
	return meshes_.size();
}

size_t ModelCompiler::Lod::totalVerts(void) const
{
	size_t total = 0;

	for (const auto& mesh : meshes_) {
		total += mesh.verts_.size();
	}

	return total;
}

size_t ModelCompiler::Lod::totalIndexs(void) const
{
	size_t total = 0;

	for (const auto& mesh : meshes_) {
		total += mesh.faces_.size();
	}

	return total * 3;
}

// ---------------------------------------------------------------

ModelCompiler::ModelCompiler(core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena, physics::IPhysicsCooking* pPhysCooker) :
	RawModel::Model(arena, pJobSys),
	pJobSys_(pJobSys),
	pPhysCooker_(pPhysCooker),
	vertexElipsion_(MERGE_VERTEX_ELIPSION),
	texcoordElipson_(MERGE_TEXCOORDS_ELIPSION),
	jointWeightThreshold_(JOINT_WEIGHT_THRESHOLD),
	scale_(1.f),
	flags_(DEFAULT_FLAGS),
	stats_(arena)
{
	core::zero_object(lodDistances_);
}

void ModelCompiler::SetVertexElipson(float elipson)
{
	vertexElipsion_ = elipson;
}

void ModelCompiler::SetTexCoordElipson(float elipson)
{
	texcoordElipson_ = elipson;
}

void ModelCompiler::SetJointWeightThreshold(float threshold)
{
	jointWeightThreshold_ = threshold;
}

void ModelCompiler::SetScale(float scale)
{
	scale_ = scale;
}

void ModelCompiler::setFlags(CompileFlags flags)
{
	flags_ = flags;
}

void ModelCompiler::setLodDistance(float32_t dis, size_t lodIdx)
{
	X_ASSERT(lodIdx < model::MODEL_MAX_LODS, "Invalid lod index")(lodIdx, model::MODEL_MAX_LODS);
	lodDistances_[lodIdx] = dis;
}

void ModelCompiler::printStats(void) const
{
	stats_.print(flags_);
}


float ModelCompiler::getVertexElipson(void) const
{
	return vertexElipsion_;
}

float ModelCompiler::getTexCoordElipson(void) const
{
	return texcoordElipson_;
}

float ModelCompiler::getJointWeightThreshold(void) const
{
	return jointWeightThreshold_;
}

float ModelCompiler::getScale(void) const
{
	return scale_;
}

ModelCompiler::CompileFlags ModelCompiler::getFlags(void) const
{
	return flags_;
}

size_t ModelCompiler::totalMeshes(void) const
{
	size_t num = 0;
	for (auto& lod : compiledLods_) {
		num += lod.meshes_.size();
	}

	return num;
}


bool ModelCompiler::CompileModel(const core::Path<char>& outFile)
{
	return CompileModel(core::Path<wchar_t>(outFile));
}


bool ModelCompiler::CompileModel(const core::Path<wchar_t>& outFile)
{
	stats_.clear();
		
	core::Path<wchar_t> path(outFile);
	path.setExtension(model::MODEL_FILE_EXTENSION_W);

	// raw models are unprocessed and un optimised.
	// if you want to make a engine model it must first be loaded into a raw model
	// that way the opermisation and format writer logic can all be in one place.
	// So i don't have to update the maya plugin and the converter when i edit the model format or change optermisations.
	{
		core::StopWatch timer;

		if (!ProcessModel()) {
			X_ERROR("Model", "Failed to compile model");
			return false;
		}

		stats_.compileTime = timer.GetTimeVal();
	}

	// save it.
	if (!SaveModel(path)) {
		X_ERROR("Model", "Failed to save compiled model to: \"%ls\"",
			path.c_str());
		return false;
	}

	compiledLods_.clear();
	return true;
}

bool ModelCompiler::SaveModel(core::Path<wchar_t>& outFile)
{
	// open da file!
	core::fileModeFlags mode;
	mode.Set(core::fileMode::WRITE);
	mode.Set(core::fileMode::RECREATE);

	core::XFileScoped file;
		
	if (!file.openFile(outFile.c_str(), mode)) {
		X_ERROR("Model", "Failed to open compile output file");
		return false;
	}

	stats_.totalJoints = safe_static_cast<uint8_t, size_t>(bones_.size());


	core::ByteStream stream(arena_);

	model::ModelHeader header;
	core::zero_object(header);

	Flags8<model::StreamType> streamsFlags;
	streamsFlags.Set(model::StreamType::COLOR);
	streamsFlags.Set(model::StreamType::NORMALS);
	streamsFlags.Set(model::StreamType::TANGENT_BI);

	header.version = model::MODEL_VERSION;
	header.flags.Set(model::ModelFlags::LOOSE);
	header.flags.Set(model::ModelFlags::STREAMS);
	header.numBones = safe_static_cast<uint8_t, size_t>(bones_.size());
	header.numBlankBones = bones_.isEmpty() ? 1 : 0; // add blank if no bones.
	header.numLod = safe_static_cast<uint8_t, size_t>(compiledLods_.size());
	header.numMesh = safe_static_cast<uint8_t, size_t>(totalMeshes());
	header.modified = core::dateTimeStampSmall::systemDateTime();

	header.vertexFmt = render::shader::VertexFormat::P3F_T2S;

	if (streamsFlags.IsSet(model::StreamType::COLOR) 
		&& !streamsFlags.IsSet(model::StreamType::NORMALS)
		&& !streamsFlags.IsSet(model::StreamType::TANGENT_BI)) {
		header.vertexFmt = render::shader::VertexFormat::P3F_T2S_C4B;
	}
	else if (streamsFlags.IsSet(model::StreamType::NORMALS)
		&& streamsFlags.IsSet(model::StreamType::NORMALS)
		&& !streamsFlags.IsSet(model::StreamType::TANGENT_BI)) {
		header.vertexFmt = render::shader::VertexFormat::P3F_T2S_C4B_N3F;
	}
	else if (streamsFlags.IsSet(model::StreamType::TANGENT_BI)
		&& streamsFlags.IsSet(model::StreamType::NORMALS)
		&& streamsFlags.IsSet(model::StreamType::TANGENT_BI)) {
		header.vertexFmt = render::shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F;
	}
	else {
		X_ASSERT_NOT_IMPLEMENTED();
		return false;
	}

	// Sizes
	header.tagNameDataSize = safe_static_cast<uint16_t, size_t>(
		this->calculateTagNameDataSize());
	header.materialNameDataSize = safe_static_cast<uint16_t, size_t>(
		this->calculateMaterialNameDataSize());
	header.boneDataSize = safe_static_cast<uint16_t, size_t>(
		this->calculateBoneDataSize());
	header.subDataSize = safe_static_cast<uint32_t, size_t>(
		this->calculateSubDataSize(streamsFlags));
	header.dataSize = (header.subDataSize +
		header.tagNameDataSize + header.materialNameDataSize);


	size_t meshHeadOffsets = sizeof(model::ModelHeader);
	for (auto& bone : bones_) {
		meshHeadOffsets += bone.name_.length() + sizeof(XQuatCompressedf) + sizeof(Vec3f) + 2;
	}

	for (size_t i = 0; i < compiledLods_.size(); i++) {
		model::LODHeader& lod = header.lodInfo[i];
		lod.lodDistance = compiledLods_[i].distance_;
		lod.numSubMeshes = safe_static_cast<uint16_t, size_t>(compiledLods_[i].numMeshes());
		// we want to know the offset o.o
		lod.subMeshHeads = meshHeadOffsets;

		// version 5.0 info
		lod.numVerts = safe_static_cast<uint32_t, size_t>(compiledLods_[i].totalVerts());
		lod.numIndexes = safe_static_cast<uint32_t, size_t>(compiledLods_[i].totalIndexs());

		// Version 8.0 info
		lod.streamsFlag = streamsFlags;

		// work out bounds for all meshes.
		lod.boundingBox.clear();
		for (size_t x = 0; x < compiledLods_[i].meshes_.size(); x++)
		{
			const Mesh& mesh = compiledLods_[i].meshes_[x];

			lod.boundingBox.add(mesh.boundingBox_);
		}
		// create sphere.
		lod.boundingSphere = Sphere(lod.boundingBox);


		meshHeadOffsets += lod.numSubMeshes * sizeof(model::MeshHeader);
	}

	// create combined bounding box.
	header.boundingBox.clear();
	for (size_t i = 0; i < compiledLods_.size(); i++)
	{
		model::LODHeader& lod = header.lodInfo[i];
		header.boundingBox.add(lod.boundingBox);
	}

	// update bounds in stats.
	stats_.bounds = header.boundingBox;

	if (file.writeObj(header) != sizeof(header)) {
		X_ERROR("Modle", "Failed to write header");
		return false;
	}

	// material names( ALL LOD)
	{
		for (size_t i = 0; i < compiledLods_.size(); i++)
		{
			// meshes 
			for (auto& mesh : compiledLods_[i].meshes_)
			{
				RawModel::Material& mat = mesh.material_;

#if defined(X_MODEL_MTL_LOWER_CASE_NAMES) && X_MODEL_MTL_LOWER_CASE_NAMES
				mat.name_.toLower();
#endif // ~X_MODEL_MTL_LOWER_CASE_NAMES

				if (!file.writeString(mat.name_)) {
					X_ERROR("Modle", "Failed to write material name");
					return false;
				}
			}
		}
	}

	// bone data.
	{
		for (auto& bone : bones_)
		{
#if defined(X_MODEL_BONES_LOWER_CASE_NAMES) && X_MODEL_BONES_LOWER_CASE_NAMES
			bone.name_.toLower();
#endif // ~X_MODEL_BONES_LOWER_CASE_NAMES
			if (!file.writeString(bone.name_)) {
				X_ERROR("Modle", "Failed to write bone name");
				return false;
			}
		}

		// space for name index data.
		{
			uint16_t blankData[MODEL_MAX_BONES] = { 0 };
			const size_t boneNameIndexBytes = sizeof(uint16_t) * (header.numBones + header.numBlankBones);

			if (file.write(blankData, boneNameIndexBytes) != boneNameIndexBytes) {
				X_ERROR("Model", "Failed to write bone index data");
				return true;
			}
		}

		// hierarchy
		for (auto& bone : bones_)
		{
			uint8_t parent = 0;

			// got a parent?
			if (bone.parIndx_ >= 0) {
				parent = safe_static_cast<uint8_t, int32_t>(bone.parIndx_);
			}	

			if (file.writeObj(parent) != sizeof(parent)) {
				X_ERROR("Model", "Failed to write bone parent");
				return false;
			}
		}

		// angles.
		for (auto& bone : bones_)
		{
			XQuatCompressedf quat(bone.rotation_);

			if (file.writeObj(quat) != sizeof(quat)) {
				X_ERROR("Model", "Failed to write bone angle");
				return false;
			}
		}

		// pos.
		for (auto& bone : bones_)
		{
			const Vec3f& pos = bone.worldPos_;

			if (file.writeObj(pos) != sizeof(pos)) {
				X_ERROR("Model", "Failed to write bone pos");
				return false;
			}
		}
	}

	// write mesh headers for each lod.
	for (size_t i = 0; i < compiledLods_.size(); i++)
	{
		uint32_t vertOffset, indexOffset;

		vertOffset = 0;
		indexOffset = 0;

		stats_.totalLods++;

		for (auto& compiledMesh : compiledLods_[i].meshes_)
		{
			model::SubMeshHeader meshHdr;
			core::zero_object(meshHdr);

			// bind info.
			meshHdr.numBinds = safe_static_cast<uint16_t, size_t>(compiledMesh.binds_.numSimpleBinds());
			meshHdr.CompBinds.set(compiledMesh.binds_.getBindCounts());

			meshHdr.numVerts = safe_static_cast<uint16_t, size_t>(compiledMesh.verts_.size());
			meshHdr.numIndexes = safe_static_cast<uint16_t, size_t>(compiledMesh.faces_.size() * 3);
			//		mesh.material = pMesh->material;
	//		mesh.CompBinds = pMesh->CompBinds;
			meshHdr.boundingBox = compiledMesh.boundingBox_;
			meshHdr.boundingSphere = Sphere(compiledMesh.boundingBox_);

			// Version 5.0 info
			meshHdr.startVertex = vertOffset;
			meshHdr.startIndex = indexOffset;

			// Version 8.0 info
			meshHdr.streamsFlag = streamsFlags; // currently a 3d model has identical flags for all meshes.

			// inc the offsets
			vertOffset += meshHdr.numVerts;
			indexOffset += meshHdr.numIndexes;

			stats_.totalMesh++;
			stats_.totalVerts += meshHdr.numVerts;
			stats_.totalFaces += (meshHdr.numIndexes / 3);

			if ((meshHdr.numIndexes % 3) != 0) {
				X_ERROR("Model", "Mesh index count is not a multiple of 3, count: %" PRIu16, meshHdr.numIndexes);
				return false;
			}

			if (file.writeObj(meshHdr) != sizeof(meshHdr)) {
				X_ERROR("Model", "Failed to write mesh header");
				return false;
			}
		}
	}

	// now we write Vert + faces + bindData
	// we convert them now to the real format.
	// as processing the data in the commpressed state is slower.

	for (size_t i = 0; i < compiledLods_.size(); i++)
	{
		size_t requiredStreamSize;

		requiredStreamSize = compiledLods_[i].getSubDataSize(streamsFlags);

		// writing this info to a stream makes write time 5x times faster.
		stream.resize(requiredStreamSize 
			// space for max padding.
			+ (16 * 5));
		stream.reset();

		const size_t streamOffset = safe_static_cast<size_t>(file.tell() - sizeof(header));

		// this pads the stream so that the end of the stream is 16byte aligned.
		auto padStream = [streamOffset, &stream]()
		{
			const size_t curOffset = streamOffset + stream.size();
			const size_t pad = core::bitUtil::RoundUpToMultiple<size_t>(curOffset, 16u) - curOffset;
			for (size_t i = 0; i < pad; i++)
			{
				stream.write<uint8_t>(0xff);
			}

			X_ASSERT_ALIGNMENT(stream.size() + streamOffset, 16, 0);
		};


		padStream();

		// write all the verts.
		for (auto& compiledMesh : compiledLods_[i].meshes_)
		{
			model::Vertex vert;
			core::zero_object(vert);

			for (size_t x = 0; x < compiledMesh.verts_.size(); x++)
			{
				const Vert& compiledVert = compiledMesh.verts_[x];

				vert.pos = compiledVert.pos_;
				vert.st = core::XHalf2::compress(compiledVert.uv_[0], compiledVert.uv_[1]);

				stream.write(vert);
			}

			padStream();
		}

		// write all the colors.
		if (streamsFlags.IsSet(model::StreamType::COLOR))
		{
			for (auto& compiledMesh : compiledLods_[i].meshes_)
			{
				model::VertexColor col;
				col.set(0xFF, 0xFF, 0xFF, 0xFF);

				if (flags_.IsSet(CompileFlag::WHITE_VERT_COL))
				{
					for (size_t x = 0; x < compiledMesh.verts_.size(); x++)
					{
						stream.write(col);
					}
				}
				else
				{
					for (size_t x = 0; x < compiledMesh.verts_.size(); x++)
					{
						const Vert& compiledVert = compiledMesh.verts_[x];

						col = compiledVert.col_;
						stream.write(col);
					}
				}
			}

			padStream();
		}

		// write normals
		if (streamsFlags.IsSet(model::StreamType::NORMALS))
		{
			for (auto& compiledMesh : compiledLods_[i].meshes_)
			{
				model::VertexNormal normal;

				for (size_t x = 0; x < compiledMesh.verts_.size(); x++)
				{
					const Vert& rawVert = compiledMesh.verts_[x];

					normal = rawVert.normal_;
					stream.write(normal);
				}
			}

			padStream();
		}

		// write tangents and bi-normals
		if (streamsFlags.IsSet(model::StreamType::TANGENT_BI))
		{
			for (auto& compiledMesh : compiledLods_[i].meshes_)
			{
				model::VertexTangentBi tangent;

				for (size_t x = 0; x < compiledMesh.verts_.size(); x++)
				{
					const Vert& rawVert = compiledMesh.verts_[x];

					tangent.binormal = rawVert.biNormal_;
					tangent.tangent = rawVert.tangent_;
					stream.write(tangent);
				}
			}

			padStream();
		}

		// write all the faces
		for (auto& compiledMesh : compiledLods_[i].meshes_)
		{
			for (size_t x = 0; x < compiledMesh.faces_.size(); x++)
			{
				const RawModel::Face& face = compiledMesh.faces_[x];

				stream.write<model::Index>(safe_static_cast<model::Index, RawModel::Index>(face[2]));
				stream.write<model::Index>(safe_static_cast<model::Index, RawModel::Index>(face[1]));
				stream.write<model::Index>(safe_static_cast<model::Index, RawModel::Index>(face[0]));
			}
		}

		// Write the complete LOD's data all at once.
		if (file.write(stream.begin(), stream.size()) != stream.size()) {
			X_ERROR("Model", "Failed to write lod data");
			return false;
		}

		// write all the bind info.
		for (auto& compiledMesh : compiledLods_[i].meshes_)
		{
			if (!compiledMesh.binds_.write(file)) {
				X_ERROR("Model", "Failed to write bind data");
				return false;
			}
		}

	}

	return true;
}

size_t ModelCompiler::calculateTagNameDataSize(void) const
{
	size_t size = 0;

	for (auto& bone : bones_) {
		size += core::strUtil::StringBytesIncNull(bone.name_); 
	}

	return size;
}

size_t ModelCompiler::calculateMaterialNameDataSize(void) const
{
	size_t size = 0;

	for (auto& lod : compiledLods_) {
		for (auto& mesh : lod.meshes_) {
			size += core::strUtil::StringBytesIncNull(mesh.material_.name_); 
		}
	}

	return size;
}

size_t ModelCompiler::calculateSubDataSize(const Flags8<model::StreamType>& streams) const
{
	size_t size = this->calculateBoneDataSize();

	size += sizeof(model::SubMeshHeader) * totalMeshes();

	for (auto& lod : compiledLods_) {
		size += lod.getSubDataSize(streams);
	}

	return size;
}

size_t ModelCompiler::calculateBoneDataSize(void) const
{
	size_t size = 0;


	const size_t fullbones = bones_.size();
	const size_t blankBones = bones_.isEmpty() ? 1 : 0;
	const size_t totalbones = fullbones + blankBones;

	// don't store pos,angle,hier for blank bones currently.
	size += (fullbones * sizeof(uint8_t)); // hierarchy
	size += (fullbones * sizeof(XQuatCompressedf)); // angle
	size += (fullbones * sizeof(Vec3f));	// pos.
	size += (totalbones * sizeof(uint16_t));	// string table idx's

	return size;
}


bool ModelCompiler::ProcessModel(void)
{
	// ok so things are a little strange now, since rawmodel format is differnt style to 
	// compiled model, meaning we have to create diffrent representation of the data.

	// we need to create the compiled verts from the tris at somepoint
	// when is best time todo that.
	// 


	// steps:
	// 1. drop weights from the raw model verts 
	// 2. merge raw mesh that share materials
	// 3. create the verts in compiledLods_ from tris.
	// 4. merge verts
	// 5. sort the verts by bind counts.
	//		sort the faces
	//		if not a animated model sort verts for linera access?
	// 6. create bind data
	// 7. scale the model.
	//  - check for collision meshes and move them.
	// 8. calculate bounds.
	// 9. check limits
	// 10. done

	if (!DropWeights()) {
		X_ERROR("Model", "Failed to drop weights");
		return false;
	}

	if (!MergMesh()) {
		X_ERROR("Model", "Failed to merge mesh");
		return false;
	}

	if (!CreateData()) {
		X_ERROR("Model", "Failed to create mesh data");
		return true;
	}

	if (!ValidateLodDistances()) {
		X_ERROR("Model", "Lod distances are invalid");
		return false;
	}

	if (!MergVerts()) {
		X_ERROR("Model", "Failed to mergeverts");
		return false;
	}

	if (!SortVerts()) {
		X_ERROR("Model", "Failed to sort verts");
		return false;
	}

	if (!OptermizeFaces()) {
		X_ERROR("Model", "Failed to optermize faces");
		return false;
	}

	if (!CreateBindData()) {
		X_ERROR("Model", "Failed to create bind data");
		return false;
	}

	if (!ScaleModel()) {
		X_ERROR("Model", "Failed to scale model");
		return false;
	}

	if (!PrcoessCollisionMeshes()) {
		X_ERROR("Model", "Failed to process collision mesh");
		return false;
	}

	if (!UpdateMeshBounds()) {
		X_ERROR("Model", "Failed to update mesh bounds");
		return false;
	}

	if (!CheckLimits()) {
		X_ERROR("Model", "Model exceeds limits");
		return false;
	}

	return true;
}

bool ModelCompiler::DropWeights(void)
{
	core::V2::Job* pLodJobs[model::MODEL_MAX_LODS] = { nullptr };

	const uint32_t batchSize = safe_static_cast<uint32_t,size_t>(getBatchSize(sizeof(RawModel::Vert)));
	X_LOG2("Model", "using batch size of %" PRIu32, batchSize);

	droppedWeights_ = 0;

	core::Delegate<void(RawModel::Vert*, uint32_t)> del;
	del.Bind<ModelCompiler, &ModelCompiler::DropWeightsJob>(this);

	// create jobs for each mesh.
	for (size_t i=0; i<lods_.size(); i++)
	{
		auto& lod = lods_[i];
		pLodJobs[i] = pJobSys_->CreateJob(core::V2::JobSystem::EmptyJob);

		for (auto& mesh : lod.meshes_)
		{
			RawModel::Vert* pVerts = mesh.verts_.ptr();
			const uint32_t numVerts = safe_static_cast<uint32_t,size_t>(mesh.verts_.size());

			core::V2::Job* pJob = pJobSys_->parallel_for_member_child<ModelCompiler>(pLodJobs[i], del, pVerts, numVerts,
				core::V2::CountSplitter32(batchSize));

			pJobSys_->Run(pJob);
		}
	}

	for (auto* pLodJob : pLodJobs)
	{
		if (pLodJob)
		{
			pJobSys_->Wait(pLodJob);
		}
	}

	// update stats
	stats_.totalWeightsDropped = droppedWeights_;
	return true;
}

bool ModelCompiler::MergMesh(void)
{
	if (!flags_.IsSet(CompileFlag::MERGE_MESH)) {
		return true;
	}

	if (hasColMeshes()) {
		X_LOG2("Model", "Skipping mesh merge, col meshes present.");
		return true;
	}

	// mesh with same materials can be merged.
	for (auto& lod : lods_)
	{
		for (size_t j = 0; j < lod.meshes_.size(); j++)
		{
			auto& mesh = lod.meshes_[j];

			for (size_t i = 0; i < lod.meshes_.size(); i++)
			{
				auto& othMesh = lod.meshes_[i];

				if (&mesh != &othMesh)
				{
					if (mesh.material_.name_ == othMesh.material_.name_)
					{
						// want to merge and remove.
						mesh.merge(othMesh);

						lod.meshes_.removeIndex(i);

						// reset search, since remove can re-order.
						i = 0;

						stats_.totalMeshMerged++;
					}
				}
			}
		}
	}

	return true;
}

bool ModelCompiler::CreateData(void)
{
	core::V2::Job* pJobs[model::MODEL_MAX_LODS] = { nullptr };

	// we need to make verts for each tri.
	compiledLods_.resize(lods_.size(), Lod(arena_));
	
	size_t i;
	for (i = 0; i < lods_.size(); i++)
	{
		const RawModel::Lod& rawLod = lods_[i];
		Lod& lod = compiledLods_[i];

		lod.distance_ = lodDistances_[i];
		lod.meshes_.resize(rawLod.numMeshes(), Mesh(arena_));
	}

	// we need job info for every mesh.
	core::FixedArray<CreateDataJobData, MODEL_MAX_MESH> jobData[MODEL_MAX_LODS];

	for (i = 0; i < compiledLods_.size(); i++)
	{
		RawModel::Mesh* pRawMesh = lods_[i].meshes_.ptr();
		Mesh* pMesh = compiledLods_[i].meshes_.ptr();
		const size_t numMesh = compiledLods_[i].meshes_.size();

		for (size_t x = 0; x < numMesh; x++) {

			CreateDataJobData data;
			data.pMesh = &pMesh[x];
			data.pRawMesh = &pRawMesh[x];

			jobData[i].append(data);
		}

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for(jobData[i].ptr(), numMesh,
			&CreateDataJob, core::V2::CountSplitter(1));

		pJobSys_->Run(pJobs[i]);
	}

	for (i = 0; i < compiledLods_.size(); i++) {
		pJobSys_->Wait(pJobs[i]);
	}

	return true;
}


bool ModelCompiler::ValidateLodDistances(void)
{
	if (compiledLods_.size() == 1) {
		// is this right? maybe I do wnat to allow it.
		if (compiledLods_[0].distance_ != 0.f) {
			X_ERROR("Model", "LOD0 distance must be zero when only one lod");
			return false;
		}

		return true;
	}

	// check each lod distance is increasing.
	float lastDistance = compiledLods_[0].distance_;
	for (size_t i = 1; i < compiledLods_.size(); i++)
	{
		if (compiledLods_[i].distance_ <= lastDistance) {
			X_ERROR("Model", "LOD%" PRIuS " has a distance less or euqal to LOD%" PRIuS, i, i-1);
			return false;
		}

		lastDistance = compiledLods_[i].distance_;
	}

	return true;
}



bool ModelCompiler::SortVerts(void)
{
	core::V2::Job* pJobs[model::MODEL_MAX_LODS] = { nullptr };
	
	core::Delegate<void(Mesh*, uint32_t)> del;
	del.Bind<ModelCompiler, &ModelCompiler::CreateBindDataJob>(this);

	// create a job to sort each meshes verts.
	size_t i;
	for (i = 0; i < compiledLods_.size(); i++)
	{
		Mesh* pMesh = compiledLods_[i].meshes_.ptr();
		uint32_t numMesh = safe_static_cast<uint32_t, size_t>(compiledLods_[i].numMeshes());

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for_member<ModelCompiler>(del, pMesh, numMesh,core::V2::CountSplitter32(1));

		pJobSys_->Run(pJobs[i]);
	}

	for (i = 0; i < compiledLods_.size(); i++) {
		pJobSys_->Wait(pJobs[i]);
	}

	return true;
}

bool ModelCompiler::OptermizeFaces(void)
{
	if (!flags_.IsSet(CompileFlag::OPTERMIZE_FACES)) {
		return true;
	}

	FaceOptimize<uint32_t> faceOpt(arena_);

	for (size_t i = 0; i < compiledLods_.size(); i++)
	{
		Lod& lod = compiledLods_[i];

		for (size_t x = 0; x < lod.numMeshes(); x++)
		{
			Mesh& mesh = lod.meshes_[x];
			Mesh::FaceArr& faces = mesh.faces_;
			Mesh::FaceArr tmp(arena_, faces.size());

			if (faces.isEmpty()) {
				continue;
			}

			std::memcpy(tmp.data(), faces.data(), faces.size() * sizeof(Mesh::FaceArr::Type));

			faceOpt.OptimizeFaces(&tmp[0][0], faces.size() * 3, &faces[0][0], 32);
		}
	}

	return true;
}


bool ModelCompiler::CreateBindData(void)
{
	core::V2::Job* pJobs[model::MODEL_MAX_LODS] = { nullptr };

	core::Delegate<void(Mesh*, uint32_t)> del;
	del.Bind<ModelCompiler, &ModelCompiler::CreateBindDataJob>(this);

	size_t i;
	for (i = 0; i < compiledLods_.size(); i++)
	{
		Mesh* pMesh = compiledLods_[i].meshes_.ptr();
		const uint32_t numMesh = safe_static_cast<uint32_t, size_t>(compiledLods_[i].numMeshes());

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for_member<ModelCompiler>(del, pMesh, numMesh, core::V2::CountSplitter32(1));
		pJobSys_->Run(pJobs[i]);
	}

	for (i = 0; i < compiledLods_.size(); i++) {
		pJobSys_->Wait(pJobs[i]);
	}

	return true;
}


bool ModelCompiler::MergVerts(void)
{
	if (!flags_.IsSet(CompileFlag::MERGE_VERTS)) {
		return true;
	}

	core::V2::Job* pJobs[model::MODEL_MAX_LODS] = { nullptr };

	size_t i;
	size_t totalVerts = 0;

	for (i = 0; i < compiledLods_.size(); i++) {
		totalVerts += compiledLods_[i].totalVerts();
	}

	core::Delegate<void(Mesh*, uint32_t)> del;
	del.Bind<ModelCompiler, &ModelCompiler::MergeVertsJob>(this);

	for (i = 0; i < compiledLods_.size(); i++)
	{
		Mesh* pMesh = compiledLods_[i].meshes_.ptr();
		const uint32_t numMesh = safe_static_cast<uint32_t,size_t>(compiledLods_[i].numMeshes());

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for_member<ModelCompiler>(del, pMesh, numMesh, core::V2::CountSplitter32(1));
		pJobSys_->Run(pJobs[i]);
	}

	for (i = 0; i < compiledLods_.size(); i++) {
		pJobSys_->Wait(pJobs[i]);
	}

	size_t totalVertsPostMerge = 0;
	for (i = 0; i < compiledLods_.size(); i++) {
		totalVertsPostMerge += compiledLods_[i].totalVerts();
	}

	stats_.totalVertsMerged = safe_static_cast<uint32_t,size_t>(totalVerts - totalVertsPostMerge);
	return true;
}

bool ModelCompiler::ScaleModel(void)
{
	// skip if no change.
	if (math<float>::abs(scale_ - 1.f) < EPSILON_VALUEf) {
		X_LOG2("Model", "Skipping model scaling, scale is 1: %f", scale_);
		return true;
	}

	const uint32_t batchSize = safe_static_cast<uint32_t, size_t>(getBatchSize(sizeof(Vert)));
	X_LOG2("Model", "using batch size of %" PRIu32, batchSize);

	core::Delegate<void(Vert*, uint32_t)> del;
	del.Bind<ModelCompiler, &ModelCompiler::ScaleVertsJob>(this);

	auto addScaleJobForMesh = [&](core::V2::Job* pParent, auto& m) {
		Vert* pVerts = m.verts_.ptr();
		const uint32_t numVerts = safe_static_cast<uint32_t, size_t>(m.verts_.size());

		core::V2::Job* pJob = pJobSys_->parallel_for_member_child<ModelCompiler>(pParent, del,
			pVerts, numVerts, core::V2::CountSplitter32(batchSize));

		pJobSys_->Run(pJob);

	};

	core::V2::Job* pLodJobs[model::MODEL_MAX_LODS] = { nullptr };

	// create jobs for each mesh.
	for (size_t i=0; i<compiledLods_.size(); i++)
	{
		auto& lod = compiledLods_[i];

		pLodJobs[i] = pJobSys_->CreateJob(core::V2::JobSystem::EmptyJob);

		for (auto& mesh : lod.meshes_)
		{
			addScaleJobForMesh(pLodJobs[i], mesh);

			for (auto& colMesh : mesh.colMeshes_)
			{
				addScaleJobForMesh(pLodJobs[i], colMesh);
			}
		}
	}

	for (auto* pLodJob : pLodJobs)
	{
		if (pLodJob)
		{
			pJobSys_->Wait(pLodJob);
		}
	}

	return ScaleBones();
}


bool ModelCompiler::PrcoessCollisionMeshes(void)
{
	if (compiledLods_.isEmpty()) {
		return true;
	}

	auto& lod = compiledLods_[0];

	// look for any collision meshes in LOD0
	for (auto& mesh : lod.meshes_)
	{
		if (!isColisionMesh(mesh.name_))
		{
			// this is not a collision mesh, lets try find some collision mehes for it.
			for (size_t i = 0; i < lod.meshes_.size(); i++)
			{
				auto& othMesh = lod.meshes_[i];
				if (&mesh != &othMesh)
				{
					ColMeshType::Enum type;

					if (isColisionMesh(othMesh.name_, &type))
					{
						const auto colMeshMame = StripColisionPrefix(othMesh.name_);

						// we have a collision mesh
						// lets see if we can find this meshes name in it.
						if (colMeshMame.find(mesh.name_.c_str()))
						{
							// ok so this is a colmesh for Mesh.
							// lets validate it.
							// the only trailing chars we allow are _01, _02

							// move it into the meshes col meshes.
							mesh.colMeshes_.emplace_back(othMesh, type);

							lod.meshes_.removeIndex(i);

							// reset search, since remove can re-order.
							i = 0;

							stats_.totalColMesh++;
						}
					}
				}
			}
		}
	}

	// check we still don't have any.
	for (size_t i = 0; i < compiledLods_.size(); i++) {
		for (auto& mesh : compiledLods_[i].meshes_) {
			if (isColisionMesh(mesh.name_)) {
				if (i == 0) {
					X_ERROR("Model", "Failed to remove collision mesh. \"%s\"", mesh.name_.c_str());
				}
				else {
					X_ERROR("Model", "Collision meshes are only support on LOD0. \"LOG%" PRIuS ":%s\"", i, mesh.name_.c_str());
				}
				return false;
			}
		}
	}
	return true;
}



bool ModelCompiler::UpdateMeshBounds(void)
{
	core::V2::Job* pJobs[model::MODEL_MAX_LODS] = { nullptr };

	core::Delegate<void(Mesh*, uint32_t)> del;
	del.Bind<ModelCompiler, &ModelCompiler::UpdateBoundsJob>(this);

	size_t i;
	for (i = 0; i < compiledLods_.size(); i++)
	{
		Mesh* pMesh = compiledLods_[i].meshes_.ptr();
		uint32_t numMesh = safe_static_cast<uint32_t, size_t>(compiledLods_[i].numMeshes());

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for_member<ModelCompiler>(del, pMesh, numMesh, core::V2::CountSplitter32(1));
		pJobSys_->Run(pJobs[i]);
	}

	for (i = 0; i < compiledLods_.size(); i++) {
		pJobSys_->Wait(pJobs[i]);
	}

	return true;
}


bool ModelCompiler::CheckLimits(void)
{
	// check the model is below the engines limits.
	// we do this after merging, since if you create 80 meshes with all the same texture and they only added 
	// up to 2k verts there is no reason we can't support that even tho mesh limit is 64.

	if (bones_.size() > model::MODEL_MAX_BONES) {
		X_ERROR("Model", "Bone count '%" PRIuS "' exceeds limit of: %" PRIu32,
			bones_.size(), model::MODEL_MAX_BONES);
		return false;
	}

	// check the bone name lengths
	for (const auto& bone : bones_)
	{
		if (bone.name_.length() > model::MODEL_MAX_BONE_NAME_LENGTH) {
			X_ERROR("Model", "Bone name length '%s(%" PRIuS ")' exceeds limit of: %" PRIu32,
				bone.name_.c_str(), bone.name_.length(), model::MODEL_MAX_BONE_NAME_LENGTH);
			return false;
		}	

		// ensure we are lower case if it's turned on.
#if X_MODEL_BONES_LOWER_CASE_NAMES
		if (!core::strUtil::IsLower(bone.name_.begin(), bone.name_.end())) {
			X_ERROR("Model", "Source code defect bone name is not lowercase '%s'",
				bone.name_.c_str());
			return false;
		}
#endif // !X_MODEL_BONES_LOWER_CASE_NAMES
	}

	for (size_t i = 0; i < compiledLods_.size(); i++)
	{
		const auto& lod = compiledLods_[i];

		// check vert / face limits.
		size_t numVerts = lod.totalVerts();
		size_t numIndex = lod.totalIndexs();

		// print both vert and index limit issues to give more of info about the problem.
		bool invalid = false;

		if (numVerts > model::MODEL_MAX_VERTS) {
			X_ERROR("Model", "LOD(%" PRIuS ") vert count '%" PRIuS "' exceeds limit of: %" PRIu32,
				i, numVerts, model::MODEL_MAX_VERTS);
			invalid = false;
		}
		// index not face!
		if (numIndex > model::MODEL_MAX_INDEXS) {
			X_ERROR("Model", "LOD(%" PRIuS ") index count '%" PRIuS "' exceeds limit of: %" PRIu32,
				i, numIndex, model::MODEL_MAX_INDEXS);
			invalid = false;
		}

		if (invalid) {
			return false;
		}

		for (const auto& mesh : lod.meshes_)
		{
			const auto& mat = mesh.material_;

			if (mat.name_.isEmpty()) {
				X_ERROR("Model", "Source code defect material name is empty");
				return false;
			}

			if (mat.name_.length() > engine::MTL_MATERIAL_MAX_LEN) {
				X_ERROR("Model", "Material name length '%s(%" PRIuS ")' exceeds limit of: %" PRIu32 " mesh: \"%s:%s\"",
					mat.name_.c_str(), mat.name_.length(), engine::MTL_MATERIAL_MAX_LEN,
					// so some potentially helpful info.
					mesh.name_.c_str(), mesh.displayName_.c_str());
				return false;
			}

			// ensure we are lower case if it's turned on.
#if X_MODEL_MTL_LOWER_CASE_NAMES
			if (!core::strUtil::IsLower(mat.name_.begin(), mat.name_.end())) {
				X_ERROR("Model", "Source code defect material name is not lowercase '%s'",
					mat.name_.c_str());
				return false;
			}
#endif // !X_MODEL_MTL_LOWER_CASE_NAMES
		}
	}

	// check each lods.
	if (bones_.size() > model::MODEL_MAX_BONES) {
		X_ERROR("Model", "Bone count '%" PRIuS "' exceeds limit of: %" PRIu32,
			bones_.size(), model::MODEL_MAX_BONES);
		return false;
	}


	// basically impossible, but check it anyway.
	if (compiledLods_.size() > model::MODEL_MAX_LODS) {
		X_ERROR("Model", "Bone count '%" PRIuS "' exceeds limit of: %" PRIu32,
			compiledLods_.size(), model::MODEL_MAX_LODS);

		// notify me we did something impossible.
		X_ASSERT_UNREACHABLE();
		return false;
	}

	return true;
}

bool ModelCompiler::ScaleBones(void)
{
	const float scale = scale_;

	for (auto& bone : bones_)
	{
		bone.worldPos_ *= scale;
	}

	return true;
}


void ModelCompiler::MergeVertsJob(Mesh* pMesh, uint32_t count)
{
	size_t meshIdx;

	float vertexElipsion = vertexElipsion_;
	float texcoordElipson = texcoordElipson_;

	typedef std::multimap<uint64_t, Hashcontainer> hashType;
	hashType hash;

	for (meshIdx = 0; meshIdx < count; meshIdx++)
	{
		auto& mesh = pMesh[meshIdx];

		{
			hash.clear();

			Mesh::VertsArr v(arena_);

			v.swap(mesh.verts_);
			// prevent resize in 
			mesh.verts_.reserve(v.size());

			size_t numEqual = 0;
			size_t numUnique = 0;
			size_t i, x;

			RawModel::Index numVerts = safe_static_cast<RawModel::Index,size_t>(v.size());
	
			for (i = 0; i < mesh.faces_.size(); i++)
			{
				RawModel::Face& face = mesh.faces_[i];

				for (x = 0; x < 3; x++) // for each face.
				{
					const RawModel::Index& idx = face[x];
					if (idx > numVerts) {
						X_ERROR("Model", "Face index is invalid: %" PRIu32 " verts: %" PRIu32, idx, numVerts);
						return;
					}

					const Vert& vert = v[idx];
					const uint64 vert_hash = vertHash(vert);


					// is it unique?
					hashType::iterator it;
					auto ret = hash.equal_range(vert_hash);


					bool equal = false;

					for (it = ret.first; it != ret.second; ++it)
					{
						const Vert* vv = it->second.pVert;

						// quick to check size and early out.
						if (vert.binds_.size() != vv->binds_.size()) {
							continue;
						}

						if (!vert.pos_.compare(vv->pos_, vertexElipsion)) {
							continue; // not same
						}
						if (!vert.normal_.compare(vv->normal_, vertexElipsion)) {
							continue; // not same
						}
						if (!vert.uv_.compare(vv->uv_, texcoordElipson)) {
							continue; // not same
						}

						// now check if the binds are really the same.
						{
							const float32_t threshold = ModelCompiler::JOINT_WEIGHT_THRESHOLD;

							size_t x;
							for (x = 0; x < vert.binds_.size(); x++)
							{
								const RawModel::Bind& b1 = vert.binds_[x];
								const RawModel::Bind& b2 = vv->binds_[x];

								if (b1.boneIdx_ != b2.boneIdx_) {
									break;
								}
								if (math<float>::abs(b1.weight_ - b2.weight_) > threshold) {
									break;
								}
							}

							if (x != vert.binds_.size()) {
								continue;
							}
						}

						equal = true;
						break; // equal.
					}

					if (equal)
					{
						face[x] = it->second.idx;

						numEqual++;
					}
					else
					{
						numUnique++;

						size_t vertIdx = mesh.verts_.append(vert);

						face[x] = safe_static_cast<RawModel::Index, size_t>(vertIdx);

						Hashcontainer temp;
						temp.pVert = &mesh.verts_[vertIdx];
						temp.idx = face[x];

						// add hash.
						hash.insert(hashType::value_type(vert_hash, temp));
					}

				}
			}
		}
	}
}


void ModelCompiler::UpdateBoundsJob(Mesh* pMesh, uint32_t count)
{
	size_t i;

	for (i = 0; i < count; i++) {
		pMesh[i].calBoundingbox();
	}
}

void ModelCompiler::ScaleVertsJob(Vert* pVerts, uint32_t count)
{
	size_t i;
	const float scale = scale_;

	for (i = 0; i < count; i++)
	{
		auto& vert = pVerts[i];

		vert.pos_ *= scale;
	}
}

void ModelCompiler::CreateBindDataJob(Mesh* pMesh, uint32_t count)
{
	size_t meshIdx;

	for (meshIdx = 0; meshIdx < count; meshIdx++)
	{
		auto& mesh = pMesh[meshIdx];

		mesh.binds_.populate(mesh.verts_);
	}
}

void ModelCompiler::DropWeightsJob(RawModel::Vert* pVerts, uint32_t count)
{
	const size_t num = count;
	const size_t maxWeights = flags_.IsSet(CompileFlag::EXT_WEIGHTS) ? ModelCompiler::VERTEX_MAX_WEIGHTS : 4;
	const float32_t threshold = ModelCompiler::JOINT_WEIGHT_THRESHOLD;


	int32_t droppedWeights = 0;
	size_t i;
	for (i = 0; i < num; i++)
	{
		RawModel::Vert& vert = pVerts[i];
		const RawModel::Vert::BindsArr& binds = vert.binds_;

		RawModel::Vert::BindsArr finalBinds;

		for (auto bind : binds)
		{
			if (bind.weight_ > threshold)
			{
				finalBinds.append(bind);
			}
		}

		// always sort them?
		std::sort(finalBinds.begin(), finalBinds.end(), [](const RawModel::Bind& a, const RawModel::Bind& b) {
			return a.weight_ < b.weight_;
		}
		);

		while (finalBinds.size() > maxWeights)
		{
			// drop the last
			finalBinds.removeIndex(finalBinds.size() - 1);

			droppedWeights++;
		}

		vert.binds_ = finalBinds;
	}

	// update value in class.
	droppedWeights_ += droppedWeights;
}


void ModelCompiler::SortVertsJob(Mesh* pMesh, uint32_t count)
{
	size_t i;
	size_t num = count;

	typedef core::Array<RawModel::Index> IndexArray;

	// requires thread safe allocator.
	IndexArray indexs(arena_);

	for (i = 0; i < count; i++)
	{
		auto& mesh = pMesh[i];
		auto& verts = mesh.verts_;
		auto& faces = mesh.faces_;

		indexs.clear();
		indexs.resize(verts.size());
		std::iota(indexs.begin(), indexs.end(), 0);

		// sort the index's based on bounds of verts.
		std::sort(indexs.begin(), indexs.end(), [&](const IndexArray::Type& idx1, const  IndexArray::Type& idx2) {
			const auto& vert1 = verts[idx1];
			const auto& vert2 = verts[idx2];
			return vert1.binds_.size() < vert2.binds_.size();
		}
		);

		// now sort the verts.
		std::sort(verts.begin(), verts.end(), [](const Vert& a, const Vert& b) {
			return a.binds_.size() < b.binds_.size();
		}
		);

		// update all the face index's
		for (auto& f : faces)
		{
			for (size_t x = 0; x < 3; x++)// un-roll for me baby.
			{
				const IndexArray::Type origIdx = f[x];
				const IndexArray::Type newIdx = indexs[origIdx];;
				f[0] = newIdx;
			}
		}
	}
}

void ModelCompiler::CreateDataJob(CreateDataJobData* pData, size_t count)
{
	size_t meshIdx;

	for (meshIdx = 0; meshIdx < count; meshIdx++)
	{
		Mesh& mesh = *pData[meshIdx].pMesh;
		const RawModel::Mesh& rawMesh = *pData[meshIdx].pRawMesh;

		mesh.material_ = rawMesh.material_;
		mesh.name_ = rawMesh.name_;
		mesh.displayName_ = rawMesh.displayName_;

		// 3 verts for each tris.
		mesh.verts_.reserve(rawMesh.tris_.size() * 3);
		// face for each tri
		mesh.faces_.reserve(rawMesh.tris_.size());

		for (size_t j = 0; j < rawMesh.tris_.size(); j++)
		{
			const RawModel::Tri& tri = rawMesh.tris_[j];

			const RawModel::Index faceStartIdx = safe_static_cast<RawModel::Index, size_t>(mesh.verts_.size());

			for (size_t t = 0; t < 3; t++)
			{
				const RawModel::TriVert& tv = tri[t];

				// look up the vert.
				const RawModel::Vert& rv = rawMesh.verts_[tv.index_];

				Vert& vert = mesh.verts_.AddOne();
				vert.pos_ = rv.pos_;
				vert.normal_ = tv.normal_;
				vert.biNormal_ = tv.biNormal_;
				vert.tangent_ = tv.tangent_;
				vert.col_ = tv.col_;
				vert.uv_ = tv.uv_;
				vert.binds_ = rv.binds_;
			}

			// make face.
			mesh.faces_.append(RawModel::Face(
				faceStartIdx,
				faceStartIdx + 1,
				faceStartIdx + 2
			));
		}
		
	}
}

size_t ModelCompiler::getBatchSize(size_t elementSizeBytes)
{
	size_t batchSize = 0;

	core::CpuInfo* pInfo = gEnv->pCore->GetCPUInfo();

	size_t numL2info = pInfo->GetL2CacheCount();
	if (numL2info > 0) {
		// just use first info.
		const core::CpuInfo::CacheInfo& cacheInfo = pInfo->GetL2CacheInfo(0);

		batchSize = (cacheInfo.size_ / elementSizeBytes);
	}
	else {
		batchSize = 1024;
	}

	if (batchSize < 16) {
		batchSize = 16;
	}

	return batchSize;
}

RawModel::Mesh::NameString ModelCompiler::StripColisionPrefix(const RawModel::Mesh::NameString& colMeshName)
{
	if (!isColisionMesh(colMeshName)) {
		return colMeshName;
	}

	if (colMeshName.length() < 4) {
		return colMeshName;
	}

	static_assert(ColMeshType::ENUM_COUNT == 3, "Added additional col mesh types? this code needs updating");

	// lets check my + 4 logic is not broken.
	static_assert(sizeof(model::MODEL_MESH_COL_BOX_PREFIX) == 5, "MODEL_MESH_COL_BOX_PREFIX size changed");
	static_assert(sizeof(model::MODEL_MESH_COL_SPHERE_PREFIX) == 5, "MODEL_MESH_COL_SPHERE_PREFIX size changed");
	static_assert(sizeof(model::MODEL_MESH_COL_CONVEX_PREFIX) == 5, "MODEL_MESH_COL_CONVEX_PREFIX size changed");

	return RawModel::Mesh::NameString(colMeshName.begin() + 4, colMeshName.end());
}


X_NAMESPACE_END