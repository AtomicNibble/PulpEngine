#pragma once

#include <String\Path.h>
#include <Containers\Array.h>
#include <Time\TimeVal.h>

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
		MERGE_VERTS,
		EXT_WEIGHTS // allow 8 inf per vert.
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
		uint32_t totalVertsMerged;
		uint32_t totalFaces;
		uint32_t totalWeightsDropped;
		uint32_t totalMeshMerged;

		core::TimeVal compileTime;

		core::Array<core::StackString<128>>
			droppedBoneNames;

		AABB bounds;
	};

	X_DISABLE_WARNING(4324)

	X_ALIGNED_SYMBOL(class Vert, 64)
	{
	public:
		typedef core::FixedArray<RawModel::Bind, 12> BindsArr;
	public:
		Vert() = default;
		~Vert() = default;

		Vec3f pos_;
		Vec3f normal_;
		Vec3f tangent_;
		Vec3f biNormal_;
		Color col_;
		Vec2f uv_;

		BindsArr binds_;
	};

	X_ENABLE_WARNING(4324)

	class Binds
	{
	public:
		typedef core::Array<Vert> VertsArr;

	public:
		Binds(core::MemoryArenaBase* arena);
		~Binds() = default;

		bool write(core::XFileScoped& file);
		void populate(const VertsArr& verts);

		const size_t numSimpleBinds(void) const;
		const CompbindInfo::BindCountsArr& getBindCounts(void) const;
		const size_t dataSizeTotal(void) const;

	private:
		core::Array<simpleBind>	simple_;
		// for complex binds.
		core::ByteStream stream_;

		CompbindInfo bindInfo_;
	};

	class Mesh
	{
	public:
		typedef core::StackString<60> NameString;
		typedef core::Array<Vert> VertsArr;
		typedef core::Array<RawModel::Face> FaceArr;

	public:
		Mesh(core::MemoryArenaBase* arena);
		~Mesh() = default;

		void calBoundingbox(void);

	public:
		NameString name_;
		NameString displayName_;

		VertsArr verts_;
		FaceArr faces_;
		Binds binds_;

		RawModel::Material material_;
		AABB boundingBox_;
	};

	class Lod
	{
		typedef core::Array<Mesh> MeshArr;

	public:
		Lod(core::MemoryArenaBase* arena);
		~Lod() = default;

		size_t getSubDataSize(const Flags8<model::StreamType>& streams) const;
		size_t numMeshes(void) const;
		size_t totalVerts(void) const;
		size_t totalIndexs(void) const;

	public:
		float32_t distance_;
		MeshArr meshes_;
	};

	typedef core::FixedArray<Lod, model::MODEL_MAX_LODS> CompiledLodArr;

	struct CreateDataJobData
	{
		CreateDataJobData() {
			core::zero_this(this);
		}

		Mesh* pMesh;
		RawModel::Mesh* pRawMesh;
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
	bool CreateData(void);
	bool SortVerts(void);
	bool CreateBindData(void);
	bool MergVerts(void);
	bool ScaleModel(void);
	bool UpdateMeshBounds(void);
	bool CheckLimits(void);

private:
	bool ScaleBones(void);

	void MergeVertsJob(Mesh* pMesh, uint32_t count);
	void UpdateBoundsJob(Mesh* pMesh, uint32_t count);
	void ScaleVertsJob(Vert* pVerts, uint32_t count);
	void CreateBindDataJob(Mesh* pMesh, uint32_t count);
	void DropWeightsJob(RawModel::Vert* pVerts, uint32_t count);
	static void CreateDataJob(CreateDataJobData* pData, size_t count);
	static void SortVertsJob(Mesh* pMesh, size_t count);

	static size_t getBatchSize(size_t elementSizeBytes);

private:
	core::V2::JobSystem* pJobSys_;

	float vertexElipsion_;
	float texcoordElipson_;
	float jointWeightThreshold_;
	float scale_;

	CompileFlags flags_;
	CompiledLodArr compiledLods_;

	core::AtomicInt droppedWeights_;

protected:
	Stats stats_;
};


X_NAMESPACE_END
