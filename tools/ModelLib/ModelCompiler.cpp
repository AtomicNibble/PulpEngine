#include "stdafx.h"
#include "ModelCompiler.h"

#include <Threading\JobSystem2.h>

#include <IModel.h>

X_DISABLE_WARNING(4702)
#include <algorithm>
#include <map>
X_ENABLE_WARNING(4702)

X_NAMESPACE_BEGIN(model)

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


bool ModelCompiler::CompileModel(core::Path<char>& outFile)
{
	return CompileModel(core::Path<wchar_t>(outFile));
}


bool ModelCompiler::CompileModel(core::Path<wchar_t>& outFile)
{
	outFile.setExtension(model::MODEL_FILE_EXTENSION_W);

	// raw models are unprocessed and un optimised.
	// if you want to make a engine model it must first be loaded into a raw model
	// that way the opermisation and format writer logic can all be in one place.
	// So i don't have to update the maya plugin and the converter when i edit the model format or change optermisations.
	if (!ProcessModel()) {
		return false;
	}

	return false;
}

bool ModelCompiler::ProcessModel(void)
{
	if (!DropWeights()) {
		return false;
	}

	if (!MergMesh()) {
		return false;
	}

	if (!SortVerts()) {
		return false;
	}

	if (!MergVerts()) {
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

namespace
{

	unsigned int hashsingle(float f)
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
		int idx;
	};

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