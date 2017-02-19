#pragma once

#include <IFileSys.h>
#include <IPhysics.h>

#include <String\Path.h>
#include <Containers\Array.h>
#include <Time\TimeVal.h>

#include "RawModel.h"


X_NAMESPACE_BEGIN(model)

X_DECLARE_FLAGS(CompileFlag)(
	ZERO_ORIGIN,
	WHITE_VERT_COL,
	MERGE_MESH,
	MERGE_VERTS,
	EXT_WEIGHTS, // allow 8 inf per vert.
	OPTERMIZE_FACES,
	COOK_PHYS_MESH // cooks convex mesh, spheres and AABB are always converted.
);

typedef Flags<CompileFlag> CompileFlags;
X_DECLARE_FLAG_OPERATORS(CompileFlags);


class ModelCompiler : public RawModel::Model
{
public:
	typedef CompileFlag CompileFlag;
	typedef CompileFlags CompileFlags;

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
		void print(CompileFlags flags) const;

	public:
		uint32_t totalLods;
		uint32_t totalMesh;
		uint32_t totalColMesh;
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

	class ColMesh;
	class Mesh
	{
	public:
		typedef core::StackString<60> NameString;
		typedef core::Array<ColMesh> ColMeshArr;
		typedef core::Array<Vert> VertsArr;
		typedef core::Array<RawModel::Face> FaceArr;

	public:
		Mesh(const Mesh& mesh);
		Mesh(Mesh&& mesh);
		Mesh(core::MemoryArenaBase* arena);
		~Mesh() = default;

		bool hasColMesh(void) const;
		void calBoundingbox(void);

	public:
		core::MemoryArenaBase* arena_;
		NameString name_;
		NameString displayName_;

		VertsArr verts_;
		FaceArr faces_;
		Binds binds_;
		ColMeshArr colMeshes_;

		RawModel::Material material_;
		AABB boundingBox_;
	};

	class ColMesh : public Mesh
	{
	public:
		typedef physics::IPhysicsCooking::DataArr CookedData;

	public:
		ColMesh(const ColMesh& oth);
		ColMesh(const Mesh& oth, ColMeshType::Enum type);
		~ColMesh() = default;

		ColMeshType::Enum getType(void) const;
		const Sphere& getBoundingSphere(void) const;
		const CookedData& getCookedConvexData(void) const;
		size_t getPhysDataSize(void) const;

		bool processColMesh(physics::IPhysicsCooking* pCooker, bool cook);

	private:
		ColMeshType::Enum type_;

		// we inherit from Mesh which already has a AABB which is where the bounds are sotred 
		// when type is BOX.
		Sphere sphere_;
		CookedData cooked_;
	};

	class HitBoxShape
	{
	public:
		HitBoxShape();

		uint8_t getBoneIdx(void) const;
		HitBoxType::Enum getType(void) const;
		const Sphere& getBoundingSphere(void) const;
		const OBB& getOBB(void) const;
		size_t getHitBoxDataSize(void) const;


	private:
		uint8_t boneIdx_;
		HitBoxType::Enum type_;

		union {
			Sphere sphere_;
			OBB oob_;
		};
	};

	typedef core::Array<HitBoxShape> HitBoxShapeArr;

	class Lod
	{
		typedef core::Array<Mesh> MeshArr;

	public:
		Lod(core::MemoryArenaBase* arena);
		~Lod() = default;

		size_t getSubDataSize(const Flags8<model::StreamType>& streams) const;
		size_t getPhysDataSize(void) const;
		size_t numMeshes(void) const;
		size_t numColMeshes(void) const;
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
	ModelCompiler(core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena, physics::IPhysicsCooking* pPhysCooker = nullptr);
	~ModelCompiler() = default;

	void SetVertexElipson(float elipson);
	void SetTexCoordElipson(float elipson);
	void SetJointWeightThreshold(float elipson);
	void SetScale(float scale);
	void setFlags(CompileFlags flags);
	void setLodDistance(float32_t dis, size_t lodIdx);
	void printStats(void) const;

	float getVertexElipson(void) const;
	float getTexCoordElipson(void) const;
	float getJointWeightThreshold(void) const;
	float getScale(void) const;
	CompileFlags getFlags(void) const;

	size_t totalMeshes(void) const;
	size_t totalColMeshes(void) const;

	bool CompileModel(const core::Path<char>& outFile);
	bool CompileModel(const core::Path<wchar_t>& outFile);


private:
	bool SaveModel(core::Path<wchar_t>& outFile);

	size_t calculateTagNameDataSize(void) const;
	size_t calculateMaterialNameDataSize(void) const ;
	size_t calculateSubDataSize(const Flags8<model::StreamType>& streams) const;
	size_t calculateBoneDataSize(void) const;
	size_t calculatePhysDataSize(void) const;
	size_t calculateHitBoxDataSize(void) const;

	bool ProcessModel(void);
	bool ProcessCollisionMeshes(void);
	bool BakeCollisionMeshes(void);
	bool DropWeights(void);
	bool MergMesh(void);
	bool CreateData(void);
	bool ValidateLodDistances(void);
	bool SortVerts(void);
	bool OptermizeFaces(void);
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
	void SortVertsJob(Mesh* pMesh, uint32_t count);
	static void CreateDataJob(CreateDataJobData* pData, size_t count);

	static size_t getBatchSize(size_t elementSizeBytes);
	// static bool isColisionMesh(const RawModel::Mesh::NameString& name);
	static RawModel::Mesh::NameString StripColisionPrefix(const RawModel::Mesh::NameString& name);

private:
	core::V2::JobSystem* pJobSys_;
	physics::IPhysicsCooking* pPhysCooker_;

	float vertexElipsion_;
	float texcoordElipson_;
	float jointWeightThreshold_;
	float scale_;

	CompileFlags flags_;
	CompiledLodArr compiledLods_;
	HitBoxShapeArr hitboxShapes_;

	float lodDistances_[model::MODEL_MAX_LODS];

	core::AtomicInt droppedWeights_;

protected:
	Stats stats_;
};


X_NAMESPACE_END
