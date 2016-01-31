#pragma once

#include <String\Path.h>
#include <Containers\Array.h>

#include "RawModel.h"

X_NAMESPACE_DECLARE(model,
	namespace RawModel {
		class Model;
	}
);

X_NAMESPACE_BEGIN(model)

class ModelCompiler : public RawModel::Model
{
public:
	X_DECLARE_FLAGS(CompileFlag)(
		ZERO_ORIGIN,
		WHITE_VERT_COL,
		MERGE_MESH,
		MERGE_VERTS
	);

	typedef Flags<CompileFlag> CompileFlags;

private:
	static const float MERGE_VERTEX_ELIPSION;
	static const float MERGE_TEXCOORDS_ELIPSION;
	static const float JOINT_WEIGHT_THRESHOLD;
	static const int32_t VERTEX_MAX_WEIGHTS;
	static const CompileFlags DEFAULT_FLAGS;

public:
	struct Stats
	{
		Stats(core::MemoryArenaBase* arena);

		uint32_t totalLods;
		uint32_t totalMesh;
		uint32_t totalJoints;
		uint32_t totalJointsDropped;
		uint32_t totalVerts;
		uint32_t totalFaces;
		uint32_t totalWeightsDropped;

		core::Array<core::StackString<128>>
			droppedBoneNames;

		AABB bounds;
	};


public:
	ModelCompiler(core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena);
	~ModelCompiler() = default;

	void SetVertexElipson(float elipson);
	void SetTexCoordElipson(float elipson);
	void SetJointWeightThreshold(float elipson);
	void SetScale(float scale);
	void setFlags(CompileFlags flags);

	bool CompileModel(core::Path<char>& outFile);
	bool CompileModel(core::Path<wchar_t>& outFile);

private:
	bool ProcessModel(void);
	bool DropWeights(void);
	bool MergMesh(void);
	bool SortVerts(void);
	bool MergVerts(void);

private:
	void MergeVertsJob(RawModel::Mesh* pMesh, uint32_t count);
	static void SortVertsJob(RawModel::Mesh* pMesh, size_t count);
	static void DropWeightsJob(RawModel::Vert* pVerts, size_t count);

private:
	core::V2::JobSystem* pJobSys_;

	float vertexElipsion_;
	float texcoordElipson_;
	float jointWeightThreshold_;

	float scale_;

	CompileFlags flags_;

	Stats stats_;
};


X_NAMESPACE_END
