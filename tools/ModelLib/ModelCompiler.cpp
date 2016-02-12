#include "stdafx.h"
#include "ModelCompiler.h"

#include <Threading\JobSystem2.h>

#include <IModel.h>

X_DISABLE_WARNING(4702)
#include <algorithm>
#include <map>
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

	int64 vertHash(const RawModel::Vert& vert)
	{
		return Hash(vert.pos_[0], vert.pos_[1], vert.pos_[2]);
	}


	struct Hashcontainer
	{
		Hashcontainer() {
			pVert = nullptr;
			idx = 0;
		}

		RawModel::Vert* pVert;
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
	totalLods = 0;
	totalMesh = 0;
	totalJoints = 0;
	totalJointsDropped = 0;
	totalVerts = 0;
	totalFaces = 0;
	totalWeightsDropped = 0;
}

void ModelCompiler::Stats::print(void) const
{
	X_LOG0("Model", "Model Info:");
	X_LOG0("Model", "> Total Lods: %i", totalLods);
	X_LOG0("Model", "> Total Mesh: %i", totalMesh);
	X_LOG0("Model", "> Total Joints: %i", totalJoints);
	X_LOG0("Model", "> Total Joints Dropped: %i", totalJointsDropped);

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
	X_LOG0("Model", "> Total Verts: %i", totalVerts);
	X_LOG0("Model", "> Total Faces: %i", totalFaces);
	X_LOG0("Model", "> Total eights Dropped: %i", totalWeightsDropped);

	if (totalWeightsDropped > 0) {
		X_LOG0("Model", "!> bind weights where dropped, consider binding with max influences: 4");
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

/*
		if (stats_.unitOfMeasurement_ == PotatoOptions::INCHES) {
			info.append(" (inches)");
		}
		else {
			info.append(" (cm)");
		}
		*/
	}
}


void ModelCompiler::Stats::clear(void)
{
	totalLods = 0;
	totalMesh = 0;
	totalJoints = 0;
	totalJointsDropped = 0;
	totalVerts = 0;
	totalFaces = 0;
	totalWeightsDropped = 0;

	droppedBoneNames.clear();
	droppedBoneNames.setGranularity(16);

	bounds.clear();
}

ModelCompiler::ModelCompiler(core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena) :
	RawModel::Model(arena),
	pJobSys_(pJobSys),
	vertexElipsion_(MERGE_VERTEX_ELIPSION),
	texcoordElipson_(MERGE_TEXCOORDS_ELIPSION),
	jointWeightThreshold_(JOINT_WEIGHT_THRESHOLD),
	scale_(1.f),
	flags_(DEFAULT_FLAGS),
	stats_(arena)
{
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

void ModelCompiler::PrintStats(void) const
{
	stats_.print();
}


bool ModelCompiler::CompileModel(core::Path<char>& outFile)
{
	return CompileModel(core::Path<wchar_t>(outFile));
}


bool ModelCompiler::CompileModel(core::Path<wchar_t>& outFile)
{
	stats_.clear();
		
	outFile.setExtension(model::MODEL_FILE_EXTENSION_W);

	// raw models are unprocessed and un optimised.
	// if you want to make a engine model it must first be loaded into a raw model
	// that way the opermisation and format writer logic can all be in one place.
	// So i don't have to update the maya plugin and the converter when i edit the model format or change optermisations.
	if (!ProcessModel()) {
		X_ERROR("Model", "Failed to compile model");
		return false;
	}

	// save it.
	if (!SaveModel(outFile)) {
		X_ERROR("Model", "Failed to save compiled model to: \"%ls\"",
			outFile.c_str());
		return false;
	}

	return true;
}

bool ModelCompiler::SaveModel(core::Path<wchar_t>& outFile)
{

	// open da file!
	core::fileModeFlags mode;
	mode.Set(core::fileMode::WRITE);
	mode.Set(core::fileMode::RECREATE);

	core::XFileScoped file;
		
	if (file.openFile(outFile.c_str(), mode)) {
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
	header.numBlankBones = 0; //  tagOrigin_.keep ? 1 : 0;
	header.numLod = safe_static_cast<uint8_t, size_t>(lods_.size());
	header.numMesh = safe_static_cast<uint8_t, size_t>(totalMeshes());
	header.modified = core::dateTimeStampSmall::systemDateTime();

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

	for (size_t i = 0; i < lods_.size(); i++) {
		model::LODHeader& lod = header.lodInfo[i];
		lod.lodDistance = lods_[i].distance_;
		lod.numSubMeshes = safe_static_cast<uint16_t, size_t>(lods_[i].numMeshes());
		// we want to know the offset o.o
		lod.subMeshHeads = meshHeadOffsets;

		// version 5.0 info
		lod.numVerts = safe_static_cast<uint8_t, size_t>(lods_[i].totalVerts());
		lod.numIndexes = safe_static_cast<uint8_t, size_t>(lods_[i].totalIndexs());

		// Version 8.0 info
		lod.streamsFlag = streamsFlags;

		// work out bounds for all meshes.
		lod.boundingBox.clear();
		for (size_t x = 0; x < lods_[i].meshes_.size(); x++)
		{
			const RawModel::Mesh& mesh = lods_[i].meshes_[x];

			lod.boundingBox.add(mesh.boundingBox_);
		}
		// create sphere.
		lod.boundingSphere = Sphere(lod.boundingBox);


		meshHeadOffsets += lod.numSubMeshes * sizeof(model::MeshHeader);
	}

	// create combined bounding box.
	header.boundingBox.clear();
	for (size_t i = 0; i < lods_.size(); i++)
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
		for (size_t i = 0; i < lods_.size(); i++)
		{
			// meshes 
			for (auto& mesh : lods_[i].meshes_)
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
			uint16_t blankData[255] = { 0 };
			const size_t boneNameIndexBytes = sizeof(uint16_t) * (header.numBones + header.numBlankBones);

			if (file.write(blankData, boneNameIndexBytes) != boneNameIndexBytes) {
				X_ERROR("Model", "Failed to write bone index data");
				return true;
			}
		}

		// hierarchy
		{


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
	for (size_t i = 0; i < lods_.size(); i++)
	{
		uint32_t vertOffset, indexOffset;

		vertOffset = 0;
		indexOffset = 0;

		stats_.totalLods++;

		for (auto& rawMesh : lods_[i].meshes_)
		{
			model::SubMeshHeader meshHdr;
			core::zero_object(meshHdr);


			meshHdr.numBinds = 0;
			meshHdr.numVerts = safe_static_cast<uint16_t, size_t>(rawMesh.verts_.size());
			meshHdr.numIndexes = safe_static_cast<uint16_t, size_t>(rawMesh.face_.size() * 3);
			//		mesh.material = pMesh->material;
	//		mesh.CompBinds = pMesh->CompBinds;
			meshHdr.boundingBox = rawMesh.boundingBox_;
			meshHdr.boundingSphere = Sphere(rawMesh.boundingBox_);

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
				X_ERROR("Model", "Mesh index count is not a multiple of 3, count: %i", meshHdr.numIndexes);
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

	for (size_t i = 0; i < lods_.size(); i++)
	{
		size_t requiredStreamSize = 0;

		for (auto& rawMesh : lods_[i].meshes_)
		{
			//	requiredStreamSize += mesh->CompBinds.dataSizeTotal();
			requiredStreamSize += (rawMesh.face_.size() * sizeof(model::Face));
			requiredStreamSize += (rawMesh.verts_.size() * sizeof(model::Vertex));

			if (streamsFlags.IsSet(model::StreamType::COLOR))
			{
				requiredStreamSize += (rawMesh.verts_.size() * sizeof(model::VertexColor));
			}
			if (streamsFlags.IsSet(model::StreamType::NORMALS))
			{
				requiredStreamSize += (rawMesh.verts_.size() * sizeof(model::VertexNormal));
			}
			if (streamsFlags.IsSet(model::StreamType::TANGENT_BI))
			{
				requiredStreamSize += (rawMesh.verts_.size() * sizeof(model::VertexTangentBi));
			}
		}

		// writing this info to a stream makes write time 5x times faster.
		stream.resize(requiredStreamSize);
		stream.reset();

		// write all the verts.
		for (auto& rawMesh : lods_[i].meshes_)
		{
			model::Vertex vert;
			core::zero_object(vert);

			for (size_t x = 0; x < rawMesh.verts_.size(); x++)
			{
				const RawModel::Vert& rawVert = rawMesh.verts_[x];

				vert.pos = rawVert.pos_;
				vert.st[0] = XHalfCompressor::compress(rawVert.uv_[0]);
				vert.st[1] = XHalfCompressor::compress(1.f - rawVert.uv_[1]);

				stream.write(vert);
			}
		}

		// write all the colors.
		if (streamsFlags.IsSet(model::StreamType::COLOR))
		{
			for (auto& rawMesh : lods_[i].meshes_)
			{
				model::VertexColor col;
				col.set(0xFF, 0xFF, 0xFF, 0xFF);

				if (flags_.IsSet(CompileFlag::WHITE_VERT_COL))
				{
					for (size_t x = 0; x < rawMesh.verts_.size(); x++)
					{
						stream.write(col);
					}
				}
				else
				{
					for (size_t x = 0; x < rawMesh.verts_.size(); x++)
					{
						const RawModel::Vert& rawVert = rawMesh.verts_[x];

						col = rawVert.col_;
						stream.write(col);
					}
				}
			}
		}

		// write normals
		if (streamsFlags.IsSet(model::StreamType::NORMALS))
		{
			for (auto& rawMesh : lods_[i].meshes_)
			{
				model::VertexNormal normal;

				for (size_t x = 0; x < rawMesh.verts_.size(); x++)
				{
					const RawModel::Vert& rawVert = rawMesh.verts_[x];

					normal = rawVert.normal_;
					stream.write(normal);
				}
			}
		}

		// write tangents and bi-normals
		if (streamsFlags.IsSet(model::StreamType::TANGENT_BI))
		{
			for (auto& rawMesh : lods_[i].meshes_)
			{
				model::VertexTangentBi tangent;

				for (size_t x = 0; x < rawMesh.verts_.size(); x++)
				{
					const RawModel::Vert& rawVert = rawMesh.verts_[x];

					tangent.binormal = rawVert.biNormal_;
					tangent.tangent = rawVert.tangent_;
					stream.write(tangent);
				}
			}
		}

		// write all the faces
		for (auto& rawMesh : lods_[i].meshes_)
		{
			for (size_t x = 0; x < rawMesh.face_.size(); x++)
			{
				const RawModel::Face& face = rawMesh.face_[x];

				stream.write<model::Index>(safe_static_cast<model::Index, int32_t>(face[2]));
				stream.write<model::Index>(safe_static_cast<model::Index, int32_t>(face[1]));
				stream.write<model::Index>(safe_static_cast<model::Index, int32_t>(face[0]));
			}
		}

		// write all the bind info.
		for (auto& rawMesh : lods_[i].meshes_)
		{

		}

		// Write the complete LOD's data all at once.
		if (file.write(stream.begin(), stream.size()) != stream.size()) {
			X_ERROR("Model", "Failed to write lod data");
			return false;
		}
	}


	return true;
}

size_t ModelCompiler::calculateTagNameDataSize(void) const
{
	size_t size = 0;

	for (auto& bone : bones_) {
		size += bone.name_.length() + 1; // nt
	}

	return size;
}

size_t ModelCompiler::calculateMaterialNameDataSize(void) const
{
	size_t size = 0;

	for (auto& lod : lods_) {
		for (auto& mesh : lod.meshes_) {
			size += mesh.material_.name_.size() + 1; // nt
		}
	}

	return size;
}

size_t ModelCompiler::calculateSubDataSize(const Flags8<model::StreamType>& streams) const
{
	size_t size = this->calculateBoneDataSize();

	size += sizeof(model::SubMeshHeader) * totalMeshes();

	for (auto& lod : lods_) {
		size += lod.getSubDataSize(streams);
	}

	return size;
}

size_t ModelCompiler::calculateBoneDataSize(void) const
{
	size_t size = 0;


	size_t fullbones = bones_.size();
	size_t totalbones = fullbones; // +(tagOrigin_.keep ? 1 : 0);

	// don't store pos,angle,hier for blank bones currently.
	size += (fullbones * sizeof(uint8_t)); // hierarchy
	size += (fullbones * sizeof(XQuatCompressedf)); // angle
	size += (fullbones * sizeof(Vec3f));	// pos.
	size += (totalbones * sizeof(uint16_t));	// string table idx's

	return size;
}


bool ModelCompiler::ProcessModel(void)
{
	if (!DropWeights()) {
		X_ERROR("Model", "Failed to drop weights");
		return false;
	}

	if (!MergMesh()) {
		X_ERROR("Model", "Failed to merge mesh");
		return false;
	}

	if (!SortVerts()) {
		X_ERROR("Model", "Failed to sort verts");
		return false;
	}

	if (!MergVerts()) {
		X_ERROR("Model", "Failed to mergeverts");
		return false;
	}

	if (!UpdateMeshBounds()) {
		X_ERROR("Model", "Failed to update mesh bounds");
		return false;
	}

	return true;
}

bool ModelCompiler::DropWeights(void)
{
	core::Stack<core::V2::Job*> jobs(arena_);

	// create jobs for each mesh.
	for (auto lod : lods_)
	{
		for (auto mesh : lod.meshes_)
		{
			RawModel::Vert* pVerts = mesh.verts_.ptr();
			size_t numVerts = mesh.verts_.size();

			core::V2::Job* pJob = pJobSys_->parallel_for(pVerts, numVerts,
				&DropWeightsJob, core::V2::CountSplitter(1024));

			pJobSys_->Run(pJob);

			jobs.push(pJob);
		}
	}

	while (jobs.isNotEmpty())
	{
		core::V2::Job* pJob = jobs.top();
		pJobSys_->Wait(pJob);
		jobs.pop();
	}

	return false;
}

bool ModelCompiler::MergMesh(void)
{
	if (!flags_.IsSet(CompileFlag::MERGE_MESH)) {
		return true;
	}

	// mesh with same materials can be merged.
	for (auto lod : lods_)
	{
		for (auto mesh : lod.meshes_)
		{
			for (size_t i = 0; i < lod.meshes_.size(); i++)
			{
				auto othMesh = lod.meshes_[i];

				if (&mesh != &othMesh)
				{
					if (mesh.material_.name_ == mesh.material_.name_)
					{
						// want to merge and remove.
						mesh.merge(othMesh);

						lod.meshes_.removeIndex(i);
					}
				}
			}
		}
	}

	return true;
}


bool ModelCompiler::SortVerts(void)
{
	core::V2::Job* pJobs[model::MODEL_MAX_LODS] = { nullptr };
	
	// create a job to sort each meshes verts.
	size_t i;
	for (i = 0; i < lods_.size(); i++)
	{
		RawModel::Mesh* pMesh = lods_[i].meshes_.ptr();
		size_t numMesh = lods_[i].meshes_.size();

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for(pMesh, numMesh,
			&SortVertsJob,core::V2::CountSplitter(1));

		pJobSys_->Run(pJobs[i]);
	}

	for (i = 0; i < lods_.size(); i++)
	{
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
	for (i = 0; i < lods_.size(); i++)
	{
		RawModel::Mesh* pMesh = lods_[i].meshes_.ptr();
		uint32_t numMesh = safe_static_cast<uint32_t,size_t>(lods_[i].meshes_.size());

		core::Delegate<void(RawModel::Mesh*, uint32_t)> del;
		del.Bind<ModelCompiler, &ModelCompiler::MergeVertsJob>(this);

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for_member<ModelCompiler>(del, pMesh, numMesh, core::V2::CountSplitter32(1));
		pJobSys_->Run(pJobs[i]);
	}

	for (i = 0; i < lods_.size(); i++)
	{
		pJobSys_->Wait(pJobs[i]);
	}

	return true;
}

bool ModelCompiler::UpdateMeshBounds(void)
{
	core::V2::Job* pJobs[model::MODEL_MAX_LODS] = { nullptr };

	size_t i;
	for (i = 0; i < lods_.size(); i++)
	{
		RawModel::Mesh* pMesh = lods_[i].meshes_.ptr();
		uint32_t numMesh = safe_static_cast<uint32_t, size_t>(lods_[i].meshes_.size());

		core::Delegate<void(RawModel::Mesh*, uint32_t)> del;
		del.Bind<ModelCompiler, &ModelCompiler::UpdateBoundsJob>(this);

		// create a job for each mesh.
		pJobs[i] = pJobSys_->parallel_for_member<ModelCompiler>(del, pMesh, numMesh, core::V2::CountSplitter32(1));
		pJobSys_->Run(pJobs[i]);
	}

	for (i = 0; i < lods_.size(); i++)
	{
		pJobSys_->Wait(pJobs[i]);
	}


	return true;
}


void ModelCompiler::MergeVertsJob(RawModel::Mesh* pMesh, uint32_t count)
{
	size_t meshIdx;

	float vertexElipsion = vertexElipsion_;
	float texcoordElipson = texcoordElipson_;

	typedef std::multimap<uint64_t, Hashcontainer> hashType;
	hashType hash;

	for (meshIdx = 0; meshIdx < count; meshIdx++)
	{
		RawModel::Mesh& mesh = pMesh[meshIdx];

		{
			hash.clear();

			RawModel::Mesh::VertsArr v(arena_);

			v.swap(mesh.verts_);

			size_t numEqual = 0;
			size_t numUnique = 0;
			size_t i, x;

			for (i = 0; i < mesh.face_.size(); i++)
			{
				RawModel::Face& face = mesh.face_[i];

				for (x = 0; x < 3; x++) // for each face.
				{
					const RawModel::Index& idx = face[x];
					const RawModel::Vert& vert = mesh.verts_[idx];

					const uint64 vert_hash = vertHash(vert);


					// is it unique?
					hashType::iterator it;
					auto ret = hash.equal_range(vert_hash);


					bool equal = false;

					for (it = ret.first; it != ret.second; ++it)
					{
						const RawModel::Vert* vv = it->second.pVert;

						if (vert.binds_.size() != vv->binds_.size()) {
							continue;
						}

						if (!vert.pos_.compare(vv->pos_, vertexElipsion)) {
							continue; // not same
						}
						if (!vert.uv_.compare(vv->uv_, texcoordElipson)) {
							continue; // not same
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

						face[x] = safe_static_cast<int, size_t>(mesh.verts_.append(vert));
					//	if (vert.numWeights > 0) {
					//		CompBinds[vert.numWeights - 1]++;
					//	}

						Hashcontainer temp;
						temp.pVert = &mesh.verts_[mesh.verts_.size() - 1];
						temp.idx = face[x];

						// add hash.
						hash.insert(hashType::value_type(vert_hash, temp));
					}

				}
			}
		}
	}
}


void ModelCompiler::UpdateBoundsJob(RawModel::Mesh* pMesh, uint32_t count)
{
	size_t i;

	for (i = 0; i < count; i++) {
		pMesh[i].calBoundingbox();
	}
}

void ModelCompiler::SortVertsJob(RawModel::Mesh* pMesh, size_t count)
{
	size_t i;

	for (i = 0; i < count; i++)
	{
		RawModel::Mesh& mesh = pMesh[i];
		auto& verts = mesh.verts_;

		std::sort(verts.begin(), verts.end(), [](const RawModel::Vert& a, const RawModel::Vert& b) {
				return a.binds_.size() < b.binds_.size();
			}
		);
	}
}

void ModelCompiler::DropWeightsJob(RawModel::Vert* pVerts, size_t count)
{
	const size_t num = count;
	size_t i;

	size_t maxWeights = ModelCompiler::VERTEX_MAX_WEIGHTS;
	float32_t threshold = ModelCompiler::JOINT_WEIGHT_THRESHOLD;

	for (i = 0; i < num; i++)
	{
		RawModel::Vert& vert = pVerts[i];
		RawModel::Vert::BindsArr& binds = vert.binds_;

		RawModel::Vert::BindsArr finalBinds;

		for (auto bind : binds)
		{
			if (bind.weight_ > threshold)
			{
				finalBinds.append(bind);
			}
		}

		// always sort them?
		std::sort(finalBinds.begin(), finalBinds.end(), [](const RawModel::Bind& a, const RawModel::Bind b) {
				return a.weight_ < b.weight_; 
			}
		);

		while (finalBinds.size() > maxWeights)
		{
			// drop the last
			finalBinds.removeIndex(finalBinds.size() - 1);
		}

		vert.binds_ = finalBinds;
	}
}


X_NAMESPACE_END