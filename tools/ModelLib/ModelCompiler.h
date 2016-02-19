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

		void clear(void);
		void print(void) const;

	public:
		uint32_t totalLods;
		uint32_t totalMesh;
		uint32_t totalJoints;
		uint32_t totalJointsDropped;
		uint32_t totalVerts;
		uint32_t totalFaces;
		uint32_t totalWeightsDropped;
		uint32_t totalMeshMerged;

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
	void printStats(void) const;

	bool CompileModel(core::Path<char>& outFile);
	bool CompileModel(core::Path<wchar_t>& outFile);


private:
	bool SaveModel(core::Path<wchar_t>& outFile);

	size_t calculateTagNameDataSize(void) const;
	size_t calculateMaterialNameDataSize(void) const ;
	size_t calculateSubDataSize(const Flags8<model::StreamType>& streams) const;
	size_t calculateBoneDataSize(void) const;

	bool ProcessModel(void);
	bool DropWeights(void);
	bool MergMesh(void);
	bool SortVerts(void);
	bool MergVerts(void);
	bool UpdateMeshBounds(void);

private:
	void MergeVertsJob(RawModel::Mesh* pMesh, uint32_t count);
	void UpdateBoundsJob(RawModel::Mesh* pMesh, uint32_t count);
	static void SortVertsJob(RawModel::Mesh* pMesh, size_t count);
	static void DropWeightsJob(RawModel::Vert* pVerts, size_t count);

private:
	core::V2::JobSystem* pJobSys_;

	float vertexElipsion_;
	float texcoordElipson_;
	float jointWeightThreshold_;

	float scale_;

	CompileFlags flags_;
protected:
	Stats stats_;
};


X_NAMESPACE_END
