#include "stdafx.h"
#include "ModelCompiler.h"

#include <Containers\FixedByteStream.h>
#include <Threading\JobSystem2.h>
#include <Time\StopWatch.h>
#include <Util\Cpu.h>

#include <IModel.h>
#include <IMaterial.h>
#include <IFileSys.h>

#include <Math\XSphereGen.h>

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

	template < typename Pit_, typename Cit_ >
	struct VertAccessor
	{
		typedef Pit_    Pit;
		typedef Cit_    Cit;
		inline  Cit operator() (Pit it) const {
			return &(*it).pos_.x;
		}
	};

} // namesace 



const float ModelCompiler::MERGE_VERTEX_ELIPSION = 0.05f;
const float ModelCompiler::MERGE_TEXCOORDS_ELIPSION = 0.02f;
const float ModelCompiler::JOINT_WEIGHT_THRESHOLD = 0.005f;
const int32_t ModelCompiler::VERTEX_MAX_WEIGHTS = model::MODEL_MAX_VERT_BINDS;
const ModelCompiler::CompileFlags ModelCompiler::DEFAULT_FLAGS = ModelCompiler::CompileFlags(ModelCompiler::CompileFlag::WHITE_VERT_COL |
	ModelCompiler::CompileFlag::MERGE_MESH);



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


bool ModelCompiler::Binds::write(core::FixedByteStreamBase& stream)
{
	if (simple_.isNotEmpty())
	{
		X_ASSERT(stream_.size() == 0, "Complex binds not empty")(stream_.size());
		stream.write(simple_.ptr(), simple_.size());
	}
	else if(stream_.size() > 0)
	{
		stream.write(stream_.data(), stream_.size());
	}

	// no binds.
	return true;
}


void ModelCompiler::Binds::populate(const VertsArr& verts)
{
	if (verts.isEmpty()) {
		return;
	}

	std::array<int32_t, MODEL_MAX_VERT_BINDS + 1> counts{};

	// work out the bind counts
	// shoving them into 1-9 index's so we don't have to branch
	for (const auto& vert : verts) 
	{
		const size_t numBinds = vert.binds_.size();
		++counts[numBinds];
	}

	// no binds?
	if (counts[0] == verts.size()) {
		return;
	}

	CompbindInfo::BindCountsArr bindCounts{};
	for (size_t i = 0; i < MODEL_MAX_VERT_BINDS; i++)
	{
		bindCounts[i] = counts[i + 1];
	}

	/*
		counts[0] == 0 bind

		bindCounts[0] == 1 bind
		bindCounts[1] == 2 bind
	*/

	// simple binds?
	if (bindCounts[0] == verts.size())
	{
		// set bind info to zero, single binds in that are diffrent to simple binds.
		bindCounts.fill(0);
		bindInfo_.set(bindCounts);

		int32_t curBoneIdx = verts[0].binds_[0].boneIdx_; // safe

		// setup first vert.
		simpleBind sb;
		sb.jointIdx = safe_static_cast<uint16_t>(curBoneIdx);
		sb.startVert = 0;
		sb.numVerts = 0;

		size_t i, lastVertIdx = 0;
		for (i = 1; i < verts.size(); i++)
		{
			const Vert& vert = verts[i];
			const RawModel::Bind& bind = vert.binds_[0];

			if (bind.boneIdx_ != curBoneIdx)
			{
				size_t numVerts = i - lastVertIdx;

				sb.numVerts = safe_static_cast<uint16_t>(numVerts);

				simple_.append(sb);

				sb.jointIdx = bind.boneIdx_;
				sb.startVert = safe_static_cast<uint16_t>(i);
				sb.numVerts = 0; // not needed more of sanity check.

				lastVertIdx = i;
				curBoneIdx = bind.boneIdx_;
			}
		}

		// trailing?
		if (lastVertIdx != (verts.size() - 1))
		{
			size_t numVerts = i - lastVertIdx;

			sb.numVerts = safe_static_cast<uint16_t>(numVerts);
			simple_.append(sb);
		}
	}
	else
	{
		const bool unBindedVerts = counts[0] != 0;

		// turn all the none binded in to root binds.
		bindCounts[0] += counts[0];
		bindInfo_.set(bindCounts);

		stream_.reserve(bindInfo_.dataSizeTotal());

#if X_DEBUG
		size_t lastBindCount = 0;
#endif

		size_t i = 0;
		bindBone rootBoneIdx(0);

		// bind to root
		while (i < verts.size() && verts[i].binds_.isEmpty())
		{
			stream_.write(rootBoneIdx);
			++i;
		}

		// user binds.
		for (; i<verts.size(); ++i)
		{
			auto& vert = verts[i];

#if X_DEBUG
			X_ASSERT(vert.binds_.isNotEmpty(), "No binds")();
			X_ASSERT(lastBindCount <= vert.binds_.size(), "Verts should be sorted by bind counts")(lastBindCount, vert.binds_.size());
			lastBindCount = vert.binds_.size();
#endif

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
	if (simple_.isNotEmpty()) {
		return simple_.size() * sizeof(SimpleBindArr::Type);
	}

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

bool ModelCompiler::Mesh::hasColMesh(void) const
{
	return colMeshes_.isNotEmpty();
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


void ModelCompiler::Mesh::calBoundingSphere(bool accurate)
{
	if (accurate)
	{
		typedef core::BoundingSphereGen <3, VertAccessor<VertsArr::ConstIterator, const float*>> BS;
		BS bs(arena_, verts_.begin(), verts_.end());

		Vec3f center(bs.center()[0], bs.center()[1], bs.center()[2]);

		boundingSphere_ = Sphere(center, bs.squaredRadius());
		return;
	}

	// not a fan.
	if (boundingBox_.isEmpty()) {
		calBoundingbox();
	}

	boundingSphere_ = Sphere(boundingBox_);
}


// ---------------------------------------------------------------


ModelCompiler::ColMesh::ColMesh(const ColMesh& oth) :
	Mesh(oth),
	type_(oth.type_),
	cooked_(oth.cooked_)
{

}



ModelCompiler::ColMesh::ColMesh(const Mesh& oth, ColMeshType::Enum type) :
	Mesh(oth.arena_),
	type_(type),
	cooked_(oth.arena_)
{

}

ColMeshType::Enum ModelCompiler::ColMesh::getType(void) const
{
	return type_;
}


const AABB& ModelCompiler::ColMesh::getBoundingBox(void) const
{
	X_ASSERT(getType() == ColMeshType::BOX, "Invalid call on non AABB mesh")();
	return boundingBox_;
}

const Sphere& ModelCompiler::ColMesh::getBoundingSphere(void) const
{
	X_ASSERT(getType() == ColMeshType::SPHERE, "Invalid call on non sphere mesh")();
	return boundingSphere_;
}

const ModelCompiler::ColMesh::CookedData& ModelCompiler::ColMesh::getCookedConvexData(void) const
{
	X_ASSERT(getType() == ColMeshType::CONVEX, "Invalid call on non convext mesh")();
	return cooked_;
}

size_t ModelCompiler::ColMesh::getPhysDataSize(void) const
{
	static_assert(ColMeshType::ENUM_COUNT == 3, "Added additional col mesh types? this code needs updating");

	switch (type_)
	{
		case ColMeshType::BOX:
			return sizeof(AABB);
		case ColMeshType::SPHERE:
			return sizeof(Sphere);
		case ColMeshType::CONVEX:
			return sizeof(CollisionConvexHdr) + cooked_.size();
		default:
			X_ASSERT_NOT_IMPLEMENTED();
			return 0;
	}
}

bool ModelCompiler::ColMesh::processColMesh(physics::IPhysicsCooking* pCooker, bool cook)
{
	static_assert(ColMeshType::ENUM_COUNT == 3, "Added additional col mesh types? this code needs updating");

	if (type_ == ColMeshType::BOX)
	{
		// ok so we want a bounding box of the mesh.
		// we can do just reuse the normal mesh aabb logic.
		calBoundingbox();
	}
	else if (type_ = ColMeshType::SPHERE)
	{
		calBoundingSphere(true);
	}
	else if (type_ = ColMeshType::CONVEX)
	{
		// i'm gonna cook you good.
		if (cook)
		{
			X_ASSERT_NOT_NULL(pCooker);

			static_assert(std::is_same<Vec3f, decltype(VertsArr::Type::pos_)>::value, "Cooking requires vec3f points");
			
			physics::TriangleMeshDesc desc;
			desc.points.pData = &verts_.front().pos_;
			desc.points.stride = sizeof(VertsArr::Type);
			desc.points.count = static_cast<uint32_t>(verts_.size());
			desc.triangles.pData = faces_.data();
			desc.triangles.stride = sizeof(FaceArr::Type);
			desc.triangles.count = safe_static_cast<uint32_t>(faces_.size());

			// we have 32bit faces.
			if (!pCooker->cookConvexMesh(desc, cooked_))
			{
				X_ERROR("Model", "Failed to cook convex physics mesh");
				return false;
			}

			if (cooked_.size() > MODEL_MESH_COL_MAX_COOKED_SIZE)
			{
				X_ERROR("Model", "Cooked convex mesh is too big: %" PRIuS " max allowed size in bytes: %" PRIuS,
					cooked_.size(), MODEL_MESH_COL_MAX_COOKED_SIZE);
				return false;
			}
		}
		else
		{
			// we pack the raw data here, so writing out the convext mesh data once compiled is the same
			// if it's baked or not.
			const size_t requiredBytes = (sizeof(Vec3f) * verts_.size()) + (sizeof(uint16_t) * faces_.size());

			// we need the verts to be Vec3f otherwise i need to convert them here.
			// we pack the faces into 16bit since there can only be 255 verts.
			// meaning we could pack as 8bit, but just makes the runtime code more messy.
			static_assert(std::is_same<decltype(VertsArr::Type::pos_), Vec3f>::value,
				"Raw convex points need to be vec3f, convert them here is you want to change how stored in VertsArr");

			core::ByteStream stream(cooked_.getArena());
			stream.reserve(requiredBytes);

			for (const auto& v : verts_)
			{
				stream.write(v.pos_);
			}

			for (const auto& f : faces_)
			{
				Vec3<uint16_t> packedFace;
				packedFace[0] = static_cast<uint16_t>(f[0]);
				packedFace[1] = static_cast<uint16_t>(f[1]);
				packedFace[2] = static_cast<uint16_t>(f[2]);
				stream.write(packedFace);
			}

			// stream should be full.
			X_ASSERT(stream.isEos(), "Logic error")();
			X_ASSERT(stream.size() == requiredBytes, "Logic error")();


			cooked_.resize(requiredBytes);
			std::memcpy(cooked_.data(), stream.begin(), stream.size());
		}
	}
	else
	{
		X_ASSERT_UNREACHABLE();
		return false;
	}


	return true;
}

// ---------------------------------------------------------------


ModelCompiler::HitBoxShape::HitBoxShape() :
	boneIdx_(std::numeric_limits<decltype(boneIdx_)>::max())
{

}

uint8_t ModelCompiler::HitBoxShape::getBoneIdx(void) const
{
	return boneIdx_;
}

HitBoxType::Enum ModelCompiler::HitBoxShape::getType(void) const
{
	return type_;
}

const Sphere& ModelCompiler::HitBoxShape::getBoundingSphere(void) const
{
	return sphere_;
}

const OBB& ModelCompiler::HitBoxShape::getOBB(void) const
{
	return oob_;
}

size_t ModelCompiler::HitBoxShape::getHitBoxDataSize(void) const
{
	static_assert(HitBoxType::ENUM_COUNT == 2, "Added additional hitbox types? this code needs updating");

	switch (type_)
	{
		case HitBoxType::SPHERE:
			return sizeof(AABB);
		case HitBoxType::OBB:
			return sizeof(OBB);
		default:
			X_ASSERT_NOT_IMPLEMENTED();
			return 0;
	}
}

// ---------------------------------------------------------------


ModelCompiler::Lod::Lod(core::MemoryArenaBase* arena) :
	distance_(0.f),
	meshes_(arena)
{

}

size_t ModelCompiler::Lod::getMeshDataSize(const Flags8<model::StreamType>& streams) const
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
		size += safe_static_cast<size_t>(mesh.binds_.dataSizeTotal());
	}

	return size;
}

size_t ModelCompiler::Lod::getBindDataSize(void) const
{
	size_t size = 0;

	for (auto& mesh : meshes_)
	{
		size += safe_static_cast<size_t>(mesh.binds_.dataSizeTotal());
	}

	return size;
}

size_t ModelCompiler::Lod::getPhysDataSize(void) const
{
	size_t size = 0;

	for (auto& mesh : meshes_)
	{
		for (auto& colMesh : mesh.colMeshes_)
		{
			size += colMesh.getPhysDataSize();
		}
	}

	return size;
}

size_t ModelCompiler::Lod::numMeshes(void) const
{
	return meshes_.size();
}

size_t ModelCompiler::Lod::numColMeshes(void) const
{
	size_t total = 0;

	for (const auto& mesh : meshes_) {
		total += mesh.colMeshes_.size();
	}

	return total;
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
	autoColGenType_(ColGenType::BOX),
	flags_(DEFAULT_FLAGS),
	stats_(arena),
	hitboxShapes_(arena),
	relativeBoneInfo_(arena)
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

void ModelCompiler::setAutoColGenType(ColGenType::Enum type)
{
	autoColGenType_ = type;
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
		num += lod.numMeshes();
	}

	return num;
}

size_t ModelCompiler::totalColMeshes(void) const
{
	size_t num = 0;
	for (auto& lod : compiledLods_) {
		num += lod.numColMeshes();
	}

	return num;
}


bool ModelCompiler::compileModel(const core::Path<char>& outFile)
{
	return compileModel(core::Path<wchar_t>(outFile));
}


bool ModelCompiler::compileModel(const core::Path<wchar_t>& outFile)
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
	if (!saveModel(path)) {
		X_ERROR("Model", "Failed to save compiled model to: \"%ls\"",
			path.c_str());
		return false;
	}

	compiledLods_.clear();
	return true;
}

bool ModelCompiler::saveModel(core::Path<wchar_t>& outFile)
{
	// open da file!
	core::fileModeFlags mode;
	mode.Set(core::fileMode::WRITE);
	mode.Set(core::fileMode::RECREATE);

	core::XFileScoped file;

	if (!gEnv->pFileSys->createDirectoryTree(outFile.c_str())) {
		X_ERROR("Model", "Failed to create directory for output file.");
		return false;
	}

	if (!file.openFile(outFile.c_str(), mode)) {
		X_ERROR("Model", "Failed to open compile output file");
		return false;
	}

	stats_.totalJoints = safe_static_cast<uint8_t, size_t>(bones_.size());

	const size_t totalColMesh = totalColMeshes();

	model::ModelHeader header;
	core::zero_object(header);

	Flags8<model::StreamType> streamsFlags;
	streamsFlags.Set(model::StreamType::COLOR);
	streamsFlags.Set(model::StreamType::NORMALS);
	streamsFlags.Set(model::StreamType::TANGENT_BI);

	header.version = model::MODEL_VERSION;
	header.flags.Set(model::ModelFlags::LOOSE);
	header.flags.Set(model::ModelFlags::STREAMS);
	if (totalColMesh || flags_.IsSet(CompileFlag::AUTO_PHYS_SHAPES)) {
		header.flags.Set(model::ModelFlags::PHYS_DATA);

		// set it even if no convex mesh it don't matter.
		if (flags_.IsSet(CompileFlag::COOK_PHYS_MESH)) {
			header.flags.Set(model::ModelFlags::PHYS_BAKED);
		}
	}

	// any binds?
	const size_t bindDataSize = calculateBindDataSize();
	if (bindDataSize > 0) {
		header.flags.Set(model::ModelFlags::ANIMATED);
	}


	// we always have one root bone, for the kids.
	if (bones_.isEmpty())
	{
		header.numRootBones = 1;

		RawModel::Bone root;
		root.name_ = "tag_origin";
		root.parIndx_ = -1;
		root.scale_ = Vec3f::one();
		root.worldPos_ = Vec3f::zero();

		bones_.push_back(root);

		relativeBoneInfo_.push_back(RelativeBoneInfo());
	}

	// calculate root bones.
	size_t numRootBones = calculateRootBoneCount();
	header.numRootBones = safe_static_cast<uint8_t>(numRootBones);

	X_ASSERT(header.numRootBones > 0, "Must have atleast one root bone")(header.numRootBones, bones_.size());

	// check all the root bones comes first.
	for (size_t i = 0; i < numRootBones; i++)
	{
		X_ASSERT(bones_[i].parIndx_ < 0, "Expected root bone")(i, bones_[i].parIndx_);
	}


	header.numBones = safe_static_cast<uint8_t, size_t>(bones_.size());
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

	const size_t hitBoxDataSize = calculateHitBoxDataSize();

	// Sizes
	header.tagNameDataSize = safe_static_cast<uint16_t>(this->calculateTagNameDataSize());
	header.materialNameDataSize = safe_static_cast<uint16_t>(this->calculateMaterialNameDataSize());
	header.boneDataSize = safe_static_cast<uint16_t>(this->calculateBoneDataSize());
	header.meshDataSize = safe_static_cast<uint32_t>(this->calculateMeshDataSize(streamsFlags));
	header.physDataSize = safe_static_cast<uint16_t>(this->calculatePhysDataSize());
	
	// turn it into num blocks.
	static_assert(MODEL_MAX_HITBOX_DATA_SIZE / 64 <= std::numeric_limits<uint8_t>::max(), "Can't represent max hitbox data");
	if (hitBoxDataSize > 0) {
		header.hitboxDataBlocks = safe_static_cast<uint8_t>(core::bitUtil::RoundUpToMultiple(hitBoxDataSize, 64_sz) / 64);
	}
	
	header.dataSize = (
		header.tagNameDataSize + 
		header.materialNameDataSize + 
		header.boneDataSize +
		header.physDataSize + 
		(header.hitboxDataBlocks * 64) +
		header.meshDataSize
	);

	// mesh data is 16byte aligned.
	uint32_t preMeshDataPadSize = 0;
	if (((header.dataSize - header.meshDataSize) % 16) != 0) {
		preMeshDataPadSize = (16 - ((header.dataSize - header.meshDataSize) % 16));
	}

	header.dataSize += preMeshDataPadSize;
	
	// add space for padding.
	const size_t maxMeshDataPadSize = ((16 * 5) * MODEL_MAX_LODS);

	// fixed streams to make sure size calculations are correct.
	core::FixedByteStreamOwning tagNameStream(arena_, header.tagNameDataSize);
	core::FixedByteStreamOwning matNameStream(arena_, header.materialNameDataSize);
	core::FixedByteStreamOwning boneDataStream(arena_, header.boneDataSize);
	core::FixedByteStreamOwning meshDataStream(arena_, header.meshDataSize + maxMeshDataPadSize); 
	core::FixedByteStreamOwning physDataStream(arena_, header.physDataSize);
	core::FixedByteStreamOwning hitboxDataStream(arena_, header.hitboxDataBlocks * 64);


	// this seams kinda wrong.
	size_t meshHeadOffsets = sizeof(model::ModelHeader);
	meshHeadOffsets += header.boneDataSize;
	for (auto& bone : bones_) 
	{
		meshHeadOffsets += bone.name_.length(); 
	}

	for (size_t i = 0; i < compiledLods_.size(); i++) {
		const auto& compiledLod = compiledLods_[i];
		model::LODHeader& lod = header.lodInfo[i];

		lod.lodDistance = compiledLod.distance_;
		lod.numSubMeshes = safe_static_cast<uint16_t, size_t>(compiledLod.numMeshes());
		// we want to know the offset o.o
		lod.subMeshHeads = meshHeadOffsets;

		// version 5.0 info
		lod.numVerts = safe_static_cast<uint32_t, size_t>(compiledLod.totalVerts());
		lod.numIndexes = safe_static_cast<uint32_t, size_t>(compiledLod.totalIndexs());

		// Version 8.0 info
		lod.streamsFlag = streamsFlags;

		// flags
		if (compiledLod.getBindDataSize() > 0)
		{
			lod.flags.Set(MeshFlag::ANIMATED);
		}

		// work out bounds for all meshes.
		lod.boundingBox.clear();
		for (size_t x = 0; x < compiledLod.meshes_.size(); x++)
		{
			const Mesh& mesh = compiledLod.meshes_[x];

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
		const model::LODHeader& lod = header.lodInfo[i];
		header.boundingBox.add(lod.boundingBox);
	}

	// update bounds in stats.
	stats_.bounds = header.boundingBox;

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

				matNameStream.write(mat.name_.c_str(), mat.name_.length() + 1);
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
			tagNameStream.write(bone.name_.c_str(), bone.name_.length() + 1);
		}

		// space for name index data.
		{
			uint16_t blankData[MODEL_MAX_BONES] = {};
			const size_t boneNameIndexNum = (header.numBones);

			boneDataStream.write(blankData, boneNameIndexNum);
		}

		// hierarchy
		for (auto& bone : bones_)
		{
			uint8_t parent = 0;

			// got a parent?
			if (bone.parIndx_ >= 0) {
				parent = safe_static_cast<uint8_t, int32_t>(bone.parIndx_);
			}

			boneDataStream.write(parent);
		}

		// angles.
		for (auto& bone : bones_)
		{
			XQuatCompressedf quat(bone.rotation_);
			boneDataStream.write(quat);
		}

		// pos.
		for (auto& bone : bones_)
		{
			boneDataStream.write(bone.worldPos_);
		}

		// relative data.
		X_ASSERT(bones_.size() == relativeBoneInfo_.size(), "Size mismatch")();
		for (size_t i = 0; i<bones_.size(); i++)
		{
			XQuatCompressedf quat(relativeBoneInfo_[i].rotation);
			boneDataStream.write(quat);
		}

		for (size_t i = 0; i<bones_.size(); i++)
		{
			boneDataStream.write(relativeBoneInfo_[i].pos);
		}


	}

	// col data. (currently we only allow col data on lod0 so makes more sense to be seperate from lodinfo.
	if(header.flags.IsSet(model::ModelFlags::PHYS_DATA))
	{
		static_assert(std::is_trivially_constructible<CollisionInfoHdr>::value, "Potentially not safe to zero_object");

		CollisionInfoHdr colHdr;
		core::zero_object(colHdr);

		// auto shapes.
		if (flags_.IsSet(CompileFlag::AUTO_PHYS_SHAPES))
		{
			if (compiledLods_.isEmpty()) {
				X_ERROR("Model", "Auto phys mesh not supported for models with no lods"); // do we support no mesh data? dunno.
				return false;
			}

			const auto lod0 = compiledLods_[0];
			size_t lod0Mesh = lod0.numMeshes();

			core::Array<int8_t> idxMap(arena_);

			switch (autoColGenType_)
			{
				case ColGenType::BOX:
					colHdr.shapeCounts[ColMeshType::BOX] = 1;
					idxMap.append(-1);
					break;
				case ColGenType::SPHERE:
					colHdr.shapeCounts[ColMeshType::SPHERE] = 1;
					idxMap.append(-1);
					break;
				case ColGenType::PER_MESH_BOX:
					colHdr.shapeCounts[ColMeshType::BOX] = safe_static_cast<uint8_t>(lod0Mesh);
					for (size_t i = 0; i < lod0Mesh; i++) {
						idxMap.append(safe_static_cast<int8_t>(i));
					}
					break;
				case ColGenType::PER_MESH_SPHERE:
					colHdr.shapeCounts[ColMeshType::SPHERE] = safe_static_cast<uint8_t>(lod0Mesh);
					for (size_t i = 0; i < lod0Mesh; i++) {
						idxMap.append(safe_static_cast<int8_t>(i));
					}
					break;
				case ColGenType::KDOP_6:
				case ColGenType::KDOP_14:
				case ColGenType::KDOP_18:
				case ColGenType::KDOP_26:
					colHdr.shapeCounts[ColMeshType::CONVEX] = 1;
					X_ASSERT_NOT_IMPLEMENTED();
					break;
			}

			// write out the couts.
			physDataStream.write(colHdr.shapeCounts);

			// index maps. 
			physDataStream.write(idxMap.data(), idxMap.size());

			// write the shapes out
			switch (autoColGenType_)
			{
				case ColGenType::BOX:
					physDataStream.write<const AABB>(header.boundingBox);
					break;
				case ColGenType::SPHERE:
				{
					Sphere sphere(header.boundingBox); // lame sphere for now.
					physDataStream.write<const Sphere>(sphere);
					break;
				}
				case ColGenType::PER_MESH_BOX:
				{
					for (auto& mesh : lod0.meshes_)
					{
						physDataStream.write<const AABB>(mesh.boundingBox_);
					}
					break;
				}
				case ColGenType::PER_MESH_SPHERE:
				{
					for (auto& mesh : lod0.meshes_)
					{
						physDataStream.write<const Sphere>(mesh.boundingSphere_);
					}
					break;
				}
				case ColGenType::KDOP_6:
				case ColGenType::KDOP_14:
				case ColGenType::KDOP_18:
				case ColGenType::KDOP_26:
					colHdr.shapeCounts[ColMeshType::CONVEX] = 1;
					X_ASSERT_NOT_IMPLEMENTED();
					break;
			}

		}
		else
		{

			// we need to order the col mesh into type order and also create some sort of mapping to them.
			// i think i'll create like a index list and given a offset and count you can get all the indexe's into the sorted data.
			core::Array<ColMesh*> colMeshes(arena_, totalColMesh);

			for (auto& mesh : compiledLods_[0].meshes_)
			{
				for (auto& colMesh : mesh.colMeshes_)
				{
					++colHdr.shapeCounts[colMesh.getType()];

					colMeshes.push_back(&colMesh);
				}
			}

			// write out the couts.
			physDataStream.write(colHdr.shapeCounts);

			// sort them by type.
			std::sort(colMeshes.begin(), colMeshes.end(), [](const ColMesh* p1, const ColMesh* p2) {
				return p1->getType() < p2->getType();
			});

			// write the index lookup.
			{
				core::Array<int8_t> idxMap(arena_);
				idxMap.reserve(totalColMesh);

				for (auto& mesh : compiledLods_[0].meshes_)
				{
					for (auto& colMesh : mesh.colMeshes_)
					{
						// so we need to work out where are mesh has moved to and store the index.
						// so lets just find the new position.
						auto sortexIdx = colMeshes.find(&colMesh);
						if (sortexIdx == decltype(colMeshes)::invalid_index)
						{
							X_ASSERT_UNREACHABLE();
							return false;
						}

						idxMap.push_back(safe_static_cast<int8_t>(sortexIdx));
					}
				}

				physDataStream.write(idxMap.data(), idxMap.size());
			}

			// write the shapes out
			for (auto* pColMesh : colMeshes)
			{
				static_assert(ColMeshType::ENUM_COUNT == 3, "Added additional col mesh types? this code needs updating");

				switch (pColMesh->getType())
				{
					case ColMeshType::SPHERE:
						physDataStream.write<const Sphere>(pColMesh->getBoundingSphere());
						break;
					case ColMeshType::BOX:
						physDataStream.write<const AABB>(pColMesh->getBoundingBox());
						break;
					case ColMeshType::CONVEX:
					{
						const auto& data = pColMesh->getCookedConvexData();

						// maybe i should just store the 'CollisionConvexHdr' in the cooked buffer humm..
						CollisionConvexHdr convexHdr;
						if (header.flags.IsSet(model::ModelFlags::PHYS_BAKED))
						{
							static_assert(MODEL_MESH_COL_MAX_COOKED_SIZE <= std::numeric_limits<uint16_t>::max(), "Can't represent cooked convex mesh");

							// we enforce a 65k byte limit on cooked data, which should be more than enougth..
							convexHdr.dataSize = safe_static_cast<uint16_t>(data.size());
						}
						else
						{
							// the limits should ensure these are below 255, but lets check that's still true.
							static_assert(MODEL_MESH_COL_MAX_VERTS <= std::numeric_limits<uint8_t>::max(), "Can't represent col mesh verts");
							static_assert(MODEL_MESH_COL_MAX_FACE <= std::numeric_limits<uint8_t>::max(), "Can't represent col mesh faces");

							convexHdr.raw.numVerts = safe_static_cast<uint8_t>(pColMesh->verts_.size());
							convexHdr.raw.numFace = safe_static_cast<uint8_t>(pColMesh->faces_.size());
						}

						physDataStream.write(convexHdr);
						physDataStream.write(data.data(), data.size());
						break;
					}

					default:
						X_ASSERT_UNREACHABLE();
						return false;
				}
			}
		}
	}

	if (header.flags.IsSet(model::ModelFlags::HITBOX))
	{
		static_assert(std::is_trivially_constructible<HitBoxHdr>::value, "Potentially not safe to zero_object");

		HitBoxHdr hitBoxHdr;
		core::zero_object(hitBoxHdr);

		core::FixedArray<const HitBoxShape*, MODEL_MAX_BONES> sortedHitShapes;
		core::FixedArray<uint8_t, MODEL_MAX_BONES> boneIdxMap;

		for (const auto& shape : hitboxShapes_)
		{
			++hitBoxHdr.shapeCounts[shape.getType()];
			sortedHitShapes.push_back(&shape);
		}

		// sort them by type.
		std::sort(sortedHitShapes.begin(), sortedHitShapes.end(), [](const HitBoxShape* p1, const HitBoxShape* p2) {
			return p1->getType() < p2->getType();
		});

		// create the sorted boneIdx's array.
		for (const auto* pShape : sortedHitShapes)
		{
			boneIdxMap.push_back(pShape->getBoneIdx());
		}

		X_ASSERT(sortedHitShapes.size() == boneIdxMap.size(), "Sizes should match")(sortedHitShapes.size(), boneIdxMap.size());

		hitboxDataStream.write(hitBoxHdr);
		hitboxDataStream.write(boneIdxMap.data(), boneIdxMap.size());

		// now dump the shapes.
		for (const auto* pShape : sortedHitShapes)
		{
			static_assert(HitBoxType::ENUM_COUNT == 2, "Added additional hitbox types? this code needs updating");

			switch (pShape->getType())
			{
				case HitBoxType::SPHERE:
					hitboxDataStream.write<const Sphere>(pShape->getBoundingSphere());
					break;
				case HitBoxType::OBB:
					hitboxDataStream.write<const OBB>(pShape->getOBB());
					break;
				default:
					X_ASSERT_UNREACHABLE();
					return false;
			}
		}

		// now we must pad to 64 bytes, so that we can store hte block size as only 8bits.
		if ((hitBoxDataSize % 64) != 0)
		{
			const size_t padSize = 64 - (hitBoxDataSize % 64);
			char pad[64] = {};

			hitboxDataStream.write(pad, padSize);
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
			meshHdr.numBinds = safe_static_cast<uint16_t>(compiledMesh.binds_.numSimpleBinds());
			meshHdr.CompBinds.set(compiledMesh.binds_.getBindCounts());

			meshHdr.numVerts = safe_static_cast<uint16_t>(compiledMesh.verts_.size());
			meshHdr.numIndexes = safe_static_cast<uint16_t>(compiledMesh.faces_.size() * 3);
			//		mesh.material = pMesh->material;
	//		mesh.CompBinds = pMesh->CompBinds;
			meshHdr.boundingBox = compiledMesh.boundingBox_;
			meshHdr.boundingSphere = compiledMesh.boundingSphere_;

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

			meshDataStream.write(meshHdr);
		}
	}

	// now we write Vert + faces + bindData
	// we convert them now to the real format.
	// as processing the data in the commpressed state is slower.
	size_t meshDataNumPadBytes = 0;

	for (size_t i = 0; i < compiledLods_.size(); i++)
	{
		// this pads the stream so that the end of the stream is 16byte aligned.
		auto padStream = [&meshDataStream, &meshDataNumPadBytes]()
		{
			const size_t paddedSize = core::bitUtil::RoundUpToMultiple(meshDataStream.size(), 16_sz);	
			const size_t padSize = paddedSize - meshDataStream.size();

			meshDataNumPadBytes += padSize;
			meshDataStream.zeroPadToLength(paddedSize);
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

				meshDataStream.write(vert);
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
						meshDataStream.write(col);
					}
				}
				else
				{
					for (size_t x = 0; x < compiledMesh.verts_.size(); x++)
					{
						const Vert& compiledVert = compiledMesh.verts_[x];

						col = compiledVert.col_;
						meshDataStream.write(col);
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
					meshDataStream.write(normal);
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
					meshDataStream.write(tangent);
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

				meshDataStream.write<model::Index>(safe_static_cast<model::Index, RawModel::Index>(face[2]));
				meshDataStream.write<model::Index>(safe_static_cast<model::Index, RawModel::Index>(face[1]));
				meshDataStream.write<model::Index>(safe_static_cast<model::Index, RawModel::Index>(face[0]));
			}
		}

		if (header.flags.IsSet(model::ModelFlags::ANIMATED))
		{
			// write all the bind info.
			for (auto& compiledMesh : compiledLods_[i].meshes_)
			{
				if (!compiledMesh.binds_.write(meshDataStream)) {
					X_ERROR("Model", "Failed to write bind data");
					return false;
				}
			}
		}

	}

	// write everything to file.
	{
		// sanity checks.
		X_ASSERT(matNameStream.size() == header.materialNameDataSize, "Incorrect size")();
		X_ASSERT(tagNameStream.size() == header.tagNameDataSize, "Incorrect size")();
		X_ASSERT(boneDataStream.size() == header.boneDataSize, "Incorrect size")(boneDataStream.size(), header.boneDataSize);
		X_ASSERT(physDataStream.size() == header.physDataSize, "Incorrect size")();
		X_ASSERT(hitboxDataStream.size() == header.hitboxDataBlocks * 64, "Incorrect size")();
		X_ASSERT(meshDataStream.size() == (header.meshDataSize + meshDataNumPadBytes), "Incorrect size")(meshDataStream.size(), header.meshDataSize, meshDataNumPadBytes);

		X_ASSERT(matNameStream.freeSpace() == 0, "Stream incomplete")();
		X_ASSERT(tagNameStream.freeSpace() == 0, "Stream incomplete")();
		X_ASSERT(boneDataStream.freeSpace() == 0, "Stream incomplete")();
		X_ASSERT(physDataStream.freeSpace() == 0, "Stream incomplete")();
		X_ASSERT(hitboxDataStream.freeSpace() == 0, "Stream incomplete")();
		X_ASSERT(meshDataNumPadBytes <= maxMeshDataPadSize, "Padding error")();
		X_ASSERT(meshDataStream.freeSpace() == (maxMeshDataPadSize - meshDataNumPadBytes), "Stream incomplete")(meshDataStream.freeSpace(), maxMeshDataPadSize, meshDataNumPadBytes);

		header.meshDataSize += safe_static_cast<uint32_t>(meshDataNumPadBytes);
		header.dataSize += safe_static_cast<uint32_t>(meshDataNumPadBytes);

		if (file.writeObj(header) != sizeof(header)) {
			X_ERROR("Model", "Failed to write header");
			return false;
		}
		if (file.write(matNameStream.ptr(), matNameStream.size()) != matNameStream.size()) {
			X_ERROR("Model", "Failed to write mat stream");
			return false;
		}
		if (file.write(tagNameStream.ptr(), tagNameStream.size()) != tagNameStream.size()) {
			X_ERROR("Model", "Failed to write tag stream");
			return false;
		}
		if (file.write(boneDataStream.ptr(), boneDataStream.size()) != boneDataStream.size()) {
			X_ERROR("Model", "Failed to write bone stream");
			return false;
		}
		if (file.write(physDataStream.ptr(), physDataStream.size()) != physDataStream.size()) {
			X_ERROR("Model", "Failed to write phys stream");
			return false;
		}
		if (file.write(hitboxDataStream.ptr(), hitboxDataStream.size()) != hitboxDataStream.size()) {
			X_ERROR("Model", "Failed to write hitbox stream");
			return false;
		}

		// make sure this stream starts on a 16bit boundry relative to header.
		char pad[16] = {};
		if (file.write(pad, preMeshDataPadSize) != preMeshDataPadSize) {
			X_ERROR("Model", "Failed to write mesh stream");
			return false;
		}


#if X_DEBUG
		const auto fileSize = file.tell();
		const auto headerRel = (fileSize - sizeof(header));
		
		X_ASSERT((headerRel % 16) == 0, "Not aligned")(fileSize, headerRel);
#endif

		if (file.write(meshDataStream.ptr(), meshDataStream.size()) != meshDataStream.size()) {
			X_ERROR("Model", "Failed to write mesh stream");
			return false;
		}
	}


#if X_DEBUG
	X_ASSERT(file.tell() == (header.dataSize + sizeof(header)), "Incorrect header size")(file.tell(), header.dataSize + sizeof(header));
#endif // X_DEBUG

	return true;
}

size_t ModelCompiler::calculateRootBoneCount(void) const
{
	return core::accumulate(bones_.begin(), bones_.end(), 0_sz, [](const RawModel::Bone& b) -> int32_t {
		return b.parIndx_ < 0 ? 1 : 0;
	});
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

size_t ModelCompiler::calculateMeshDataSize(const Flags8<model::StreamType>& streams) const
{
	size_t size = 0;

	size += sizeof(model::SubMeshHeader) * totalMeshes();

	for (auto& lod : compiledLods_) {
		size += lod.getMeshDataSize(streams);
	}

	return size;
}

size_t ModelCompiler::calculateBindDataSize(void) const
{
	size_t size = 0;

	for (auto& lod : compiledLods_) {
		size += lod.getBindDataSize();
	}

	return size;
}

size_t ModelCompiler::calculateBoneDataSize(void) const
{
	size_t size = 0;

	const size_t totalbones = bones_.size();
//	const size_t rootBones = calculateRootBoneCount();

	// don't store pos,angle,hier for blank bones currently.
	size += (totalbones * sizeof(uint8_t)); // hierarchy
	size += (totalbones * sizeof(XQuatCompressedf)); // angle
	size += (totalbones * sizeof(Vec3f));	// pos.
	size += (totalbones * sizeof(XQuatCompressedf)); // rel angle
	size += (totalbones * sizeof(Vec3f));	// pos rel.
	size += (totalbones * sizeof(uint16_t));	// string table idx's

	return size;
}

size_t ModelCompiler::calculatePhysDataSize(void) const
{
	size_t size = 0;

	if (!totalColMeshes() && !flags_.IsSet(CompileFlag::AUTO_PHYS_SHAPES)) {
		return size;
	}

	size += sizeof(CollisionInfoHdr::shapeCounts);
	
	if (flags_.IsSet(CompileFlag::AUTO_PHYS_SHAPES))
	{
		const auto lod0 = compiledLods_[0];
		size_t lod0Mesh = lod0.numMeshes();

		switch (autoColGenType_)
		{
			case ColGenType::BOX:
				size += sizeof(AABB);
				size += sizeof(uint8_t);
				break;
			case ColGenType::SPHERE:
				size += sizeof(Sphere);
				size += sizeof(uint8_t);
				break;
			case ColGenType::PER_MESH_BOX:
				size += sizeof(AABB) * lod0Mesh;
				size += sizeof(uint8_t) * lod0Mesh;
				break;
			case ColGenType::PER_MESH_SPHERE:
				size += sizeof(Sphere) * lod0Mesh;
				size += sizeof(uint8_t) * lod0Mesh;
				break;
			case ColGenType::KDOP_6:
			case ColGenType::KDOP_14:
			case ColGenType::KDOP_18:
			case ColGenType::KDOP_26:
				X_ASSERT_NOT_IMPLEMENTED();
				break;
		}


		return size;
	}
	else
	{
		size += sizeof(uint8_t) * totalColMeshes();

		return core::accumulate(compiledLods_.begin(), compiledLods_.end(), size, [](const Lod& lod) {
			return lod.getPhysDataSize();
		});
	}
}

size_t ModelCompiler::calculateHitBoxDataSize(void) const
{
	size_t size = 0;

	if (hitboxShapes_.isEmpty()) {
		return size;
	}

	size += sizeof(HitBoxHdr);
	size += sizeof(uint8_t) * hitboxShapes_.size();

	for (const auto& shape : hitboxShapes_)
	{
		size += shape.getHitBoxDataSize();
	}

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
	
	// require cooking if flag set.
	if (flags_.IsSet(CompileFlag::COOK_PHYS_MESH) && !pPhysCooker_) {
		X_ERROR("Model", "Can't cook meshes without a cooker instance");
		return false;
	}

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

	// this won't affect sorted verts it only changes index list.
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

	if (!CreateRelativeBoneInfo()) {
		X_ERROR("Model", "Model exceeds limits");
		return false;
	}

	if (!ProcessCollisionMeshes()) {
		X_ERROR("Model", "Failed to process collision mesh");
		return false;
	}

	if (!UpdateMeshBounds()) {
		X_ERROR("Model", "Failed to update mesh bounds");
		return false;
	}

	if (!BakeCollisionMeshes()) {
		X_ERROR("Model", "Failed to bake collision mesh");
		return false;
	}

	if (!AutoCollisionGen()) {
		X_ERROR("Model", "Failed to bake collision mesh");
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

	const uint32_t batchSize = safe_static_cast<uint32_t>(getBatchSize(sizeof(RawModel::Vert)));
	X_LOG2("Model", "using batch size of %" PRIu32, batchSize);

	droppedWeights_ = 0;

	core::Delegate<void(RawModel::Vert*, uint32_t)> del;
	del.Bind<ModelCompiler, &ModelCompiler::DropWeightsJob>(this);

	// create jobs for each mesh.
	for (size_t i=0; i<lods_.size(); i++)
	{
		auto& lod = lods_[i];
		pLodJobs[i] = pJobSys_->CreateJob(core::V2::JobSystem::EmptyJob JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));

		for (auto& mesh : lod.meshes_)
		{
			RawModel::Vert* pVerts = mesh.verts_.ptr();
			const uint32_t numVerts = safe_static_cast<uint32_t>(mesh.verts_.size());

			core::V2::Job* pJob = pJobSys_->parallel_for_member_child<ModelCompiler>(pLodJobs[i], del, pVerts, numVerts,
				core::V2::CountSplitter32(batchSize) JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));

			pJobSys_->Run(pJob);
		}
	}

	for (auto* pLodJob : pLodJobs)
	{
		if (pLodJob)
		{
			pJobSys_->Run(pLodJob);
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
			&CreateDataJob, core::V2::CountSplitter(1) JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));

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
	del.Bind<ModelCompiler, &ModelCompiler::SortVertsJob>(this);

	// create a job to sort each meshes verts.
	size_t i;
	for (i = 0; i < compiledLods_.size(); i++)
	{
		Mesh* pMesh = compiledLods_[i].meshes_.ptr();
		uint32_t numMesh = safe_static_cast<uint32_t>(compiledLods_[i].numMeshes());

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for_member<ModelCompiler>(del, pMesh, numMesh,core::V2::CountSplitter32(1) JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));

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
		const uint32_t numMesh = safe_static_cast<uint32_t>(compiledLods_[i].numMeshes());

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for_member<ModelCompiler>(del, pMesh, numMesh, core::V2::CountSplitter32(1) JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));
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
		const uint32_t numMesh = safe_static_cast<uint32_t>(compiledLods_[i].numMeshes());

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for_member<ModelCompiler>(del, pMesh, numMesh, core::V2::CountSplitter32(1) JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));
		pJobSys_->Run(pJobs[i]);
	}

	for (i = 0; i < compiledLods_.size(); i++) {
		pJobSys_->Wait(pJobs[i]);
	}

	size_t totalVertsPostMerge = 0;
	for (i = 0; i < compiledLods_.size(); i++) {
		totalVertsPostMerge += compiledLods_[i].totalVerts();
	}

	stats_.totalVertsMerged = safe_static_cast<uint32_t>(totalVerts - totalVertsPostMerge);
	return true;
}

bool ModelCompiler::ScaleModel(void)
{
	// skip if no change.
	if (math<float>::abs(scale_ - 1.f) < EPSILON_VALUEf) {
		X_LOG2("Model", "Skipping model scaling, scale is 1: %f", scale_);
		return true;
	}

	const uint32_t batchSize = safe_static_cast<uint32_t>(getBatchSize(sizeof(Vert)));
	X_LOG2("Model", "using batch size of %" PRIu32, batchSize);

	core::Delegate<void(Vert*, uint32_t)> del;
	del.Bind<ModelCompiler, &ModelCompiler::ScaleVertsJob>(this);

	auto addScaleJobForMesh = [&](core::V2::Job* pParent, auto& m) {
		Vert* pVerts = m.verts_.ptr();
		const uint32_t numVerts = safe_static_cast<uint32_t>(m.verts_.size());

		core::V2::Job* pJob = pJobSys_->parallel_for_member_child<ModelCompiler>(pParent, del,
			pVerts, numVerts, core::V2::CountSplitter32(batchSize) JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));

		pJobSys_->Run(pJob);

	};

	core::V2::Job* pLodJobs[model::MODEL_MAX_LODS] = { nullptr };

	// create jobs for each mesh.
	for (size_t i=0; i<compiledLods_.size(); i++)
	{
		auto& lod = compiledLods_[i];

		pLodJobs[i] = pJobSys_->CreateJob(core::V2::JobSystem::EmptyJob JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));

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
			pJobSys_->Run(pLodJob);
			pJobSys_->Wait(pLodJob);
		}
	}

	return ScaleBones();
}

bool ModelCompiler::CreateRelativeBoneInfo(void)
{
	relativeBoneInfo_.resize(bones_.size());

	for (size_t i = 0; i < bones_.size(); i++)
	{
		const auto& bone = bones_[i];

		// skip root, already relative.
		if (bone.parIndx_ < 0) {
			continue;
		}

		const auto& parBone = bones_[bone.parIndx_];

		Vec3f relPos = bone.worldPos_ - parBone.worldPos_;
		relPos = parBone.rotation_.inverse() * relPos;

		Matrix33f relRotation = parBone.rotation_.inverse() * bone.rotation_;

		relativeBoneInfo_[i].pos = relPos;
		relativeBoneInfo_[i].rotation = relRotation;
	}

	return true;
}


bool ModelCompiler::ProcessCollisionMeshes(void)
{
	if (compiledLods_.isEmpty()) {
		return true;
	}

	// we only look for col mesh on lod0 currently.
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

							// check limits.
							if (type == ColMeshType::CONVEX)
							{
								if (othMesh.verts_.size() > MODEL_MESH_COL_MAX_VERTS)
								{
									X_ERROR("Model", "Convex col mesh exceeds vert limit of: %" PRIu32 " provided: %" PRIuS,
										MODEL_MESH_COL_MAX_VERTS, othMesh.verts_.size());
									return false;
								}
								if (othMesh.faces_.size() > MODEL_MESH_COL_MAX_FACE)
								{
									X_ERROR("Model", "Convex col mesh exceeds face limit of: %" PRIu32 " provided: %" PRIuS,
										MODEL_MESH_COL_MAX_FACE, othMesh.faces_.size());
									return false;
								}
							}

							if (mesh.colMeshes_.size() > MODEL_MESH_COL_MAX_MESH)
							{
								X_ERROR("Model", "Mesh has \"%s\" has too many physics shapes defined, max: %" PRIuS,
									mesh.name_.c_str(), MODEL_MESH_COL_MAX_MESH);
								return false;
							}

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

bool ModelCompiler::BakeCollisionMeshes(void)
{
	if (compiledLods_.isEmpty()) {
		return true;
	}

	auto& lod = compiledLods_[0];

	// look for any collision meshes in LOD0
	for (auto& lod : compiledLods_)
	{
		for (auto& mesh : lod.meshes_)
		{
			// for each mesh we can have multiple physics shapes.
			for (auto& colMesh : mesh.colMeshes_)
			{
				// process the mesh into either a AABB / sphere.
				// if it's a convex mesh we cook it.
				if (!colMesh.processColMesh(pPhysCooker_, flags_.IsSet(CompileFlag::COOK_PHYS_MESH)))
				{
					X_ERROR("Model", "Failed to process physics mesh: \"%s\"", colMesh.name_.c_str());
					return false;
				}
			}
		}
	}

	return true;
}


bool ModelCompiler::AutoCollisionGen(void)
{
	if (!flags_.IsSet(CompileFlag::AUTO_PHYS_SHAPES)) {
		return true;
	}

	if (compiledLods_.isEmpty()) {
		X_ERROR("Model", "Auto phys mesh not supported for models with no lods"); // do we support no mesh data? dunno.
		return false;
	}

	{
		size_t colMeshes = totalColMeshes();
		if (colMeshes) {
			X_WARNING("Model", "%" PRIuS " user col meshes provided when auto phys enabled, ignoring user meshes.", colMeshes);
		}
	}

	// okay my fat goat lets generat some shapes.
	ColGenType::Enum genType = ColGenType::BOX;

	switch (genType)
	{
		case ColGenType::BOX:
		case ColGenType::CAPSULE:
		case ColGenType::PER_MESH_BOX:
			break;

		// we need to gen sphere for each mesh.
		case ColGenType::PER_MESH_SPHERE:
			break;

		// gen me some kdop.
		case ColGenType::KDOP_6:
		case ColGenType::KDOP_14:
		case ColGenType::KDOP_18:
		case ColGenType::KDOP_26:
			X_ASSERT_NOT_IMPLEMENTED();
			break;
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
		uint32_t numMesh = safe_static_cast<uint32_t>(compiledLods_[i].numMeshes());

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for_member<ModelCompiler>(del, pMesh, numMesh, core::V2::CountSplitter32(1) JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));
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

			// check col mesh limits again here.
			if (mesh.colMeshes_.size() > MODEL_MESH_COL_MAX_MESH)
			{
				X_ERROR("Model", "Mesh has \"%s\" has too many physics shapes defined, max: %" PRIuS,
					mesh.name_.c_str(), MODEL_MESH_COL_MAX_MESH);
				return false;
			}

			for (const auto& colMesh : mesh.colMeshes_)
			{
				if (colMesh.getType() == ColMeshType::CONVEX)
				{
					if (colMesh.verts_.size() > MODEL_MESH_COL_MAX_VERTS)
					{
						X_ERROR("Model", "Convex col mesh exceeds vert limit of: %" PRIu32 " provided: %" PRIuS,
							MODEL_MESH_COL_MAX_VERTS, colMesh.verts_.size());
						return false;
					}
					if (colMesh.faces_.size() > MODEL_MESH_COL_MAX_FACE)
					{
						X_ERROR("Model", "Convex col mesh exceeds face limit of: %" PRIu32 " provided: %" PRIuS,
							MODEL_MESH_COL_MAX_FACE, colMesh.faces_.size());
						return false;
					}
				}
			}
		}
	}

	// basically impossible, but check it anyway.
	if (compiledLods_.size() > model::MODEL_MAX_LODS) {
		X_ERROR("Model", "Bone count '%" PRIuS "' exceeds limit of: %" PRIu32,
			compiledLods_.size(), model::MODEL_MAX_LODS);

		// notify me we did something impossible.
		X_ASSERT_UNREACHABLE();
		return false;
	}

	if (totalColMeshes() > MODEL_MAX_COL_SHAPES)
	{
		X_ERROR("Model", "total ColShape count '%" PRIuS "' exceeds per model shape limit of: %" PRIu32,
			totalColMeshes(), MODEL_MAX_COL_SHAPES);
		return false;
	}

	// check we can represent all the phys data
	if (calculatePhysDataSize() > MODEL_MAX_COL_DATA_SIZE)
	{
		X_ERROR("Model", "Cumulative size of collision data exceeds limit, try reducing convex col meshes / complexity");
		return false;
	}

	// this is a source code error tbh
	if (hitboxShapes_.size() > bones_.size())
	{
		X_ERROR("Model", "Can't have more hitbox shapes than bones.");
		return false;
	}

	// this is a source code error.
	if (hitboxShapes_.size() > MODEL_MAX_BONES)
	{
		X_ERROR("Model", "Can't have more hitbox shapes than bones.");
		return false;
	}

	const size_t hitBoxDataSize = calculateHitBoxDataSize();
	if (hitBoxDataSize > MODEL_MAX_HITBOX_DATA_SIZE)
	{
		X_ERROR("Model", "Cumulative size of hitbox data exceeds limit.");
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
	float vertexElipsion = vertexElipsion_;
	float texcoordElipson = texcoordElipson_;

	typedef std::multimap<uint64_t, Hashcontainer> hashType;
	hashType hash;

	for (uint32_t meshIdx = 0; meshIdx < count; meshIdx++)
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
	for (uint32_t i = 0; i < count; i++) {
		pMesh[i].calBoundingbox();
		pMesh[i].calBoundingSphere(false);
	}
}

void ModelCompiler::ScaleVertsJob(Vert* pVerts, uint32_t count)
{
	const float scale = scale_;

	for (uint32_t i = 0; i < count; i++)
	{
		auto& vert = pVerts[i];

		vert.pos_ *= scale;
	}
}

void ModelCompiler::CreateBindDataJob(Mesh* pMesh, uint32_t count)
{
	for (uint32_t meshIdx = 0; meshIdx < count; meshIdx++)
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
	for (size_t i = 0; i < num; i++)
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
	typedef core::Array<int32_t> IdxArray;

	// requires thread safe allocator.
	IdxArray srcToDstIdx(arena_);
	IdxArray dstToSrcIdx(arena_);

	Mesh::VertsArr tmpVerts(arena_);

	for (uint32_t i = 0; i < count; i++)
	{
		auto& mesh = pMesh[i];
		auto& verts = mesh.verts_;
		auto& faces = mesh.faces_;

		// maps source vert index to dest vert index.
		srcToDstIdx.resize(verts.size());
		// maps dest vert index to source vert index.
		dstToSrcIdx.resize(verts.size());

		std::iota(srcToDstIdx.begin(), srcToDstIdx.end(), 0);
		std::sort(srcToDstIdx.begin(), srcToDstIdx.end(),
			[&](const IdxArray::Type& idx1, const  IdxArray::Type& idx2) {
				const auto& vert1 = verts[idx1];
				const auto& vert2 = verts[idx2];
				return vert1.binds_.size() < vert2.binds_.size();
			}
		);

		tmpVerts.resize(verts.size());

		// build the reverse mapping
		for (int32_t idx = 0; idx < safe_static_cast<int32_t>(srcToDstIdx.size()); idx++)
		{
			IdxArray::Type targetIdx = srcToDstIdx[idx];
			dstToSrcIdx[targetIdx] = idx;
		}

		// created sorted verts and swap.
		for (size_t dstIdx = 0; dstIdx < dstToSrcIdx.size(); dstIdx++)
		{
			IdxArray::Type srcIdx = dstToSrcIdx[dstIdx];
			tmpVerts[srcIdx] = verts[dstIdx];
		}

		tmpVerts.swap(verts);

		// update all the face index's
		for (auto& f : faces)
		{
			for (int32_t x = 0; x < 3; x++)// un-roll for me baby.
			{
				const IdxArray::Type origIdx = f[x];
				const IdxArray::Type newIdx = dstToSrcIdx[origIdx];
				f[x] = newIdx;
			}
		}

#if X_ENABLE_ASSERTIONS
		bool sorted = std::is_sorted(verts.begin(), verts.end(),
			[&](const Vert& v1, const Vert& v2) {
				return v1.binds_.size() < v2.binds_.size();
			}
		);

		X_ASSERT(sorted, "Not sorted")(sorted);
#endif // !X_ENABLE_ASSERTIONS
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