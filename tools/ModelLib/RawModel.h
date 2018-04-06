#pragma once

#include <IScale.h>
#include <IModel.h>
#include <String\Path.h>

#include "RawModelTypes.h"

X_NAMESPACE_DECLARE(core,
    class XLexer);

X_NAMESPACE_DECLARE(core,
    namespace V2 {
        class JobSystem;
        struct Job;
    });

X_NAMESPACE_BEGIN(model)

namespace RawModel
{
    class Model
    {
        typedef core::StackString<0x20000, char> ModelDataStr;
        typedef core::UniquePointer<ModelDataStr> ModelDataStrPtr;
        typedef core::Array<ModelDataStr*> ModelDataStrArr;

        typedef core::FixedArray<Lod, model::MODEL_MAX_LODS> LodArr;
        typedef core::Array<Bone> BoneArr;

        static const int32_t VERSION;

    public:
        // need to be a thread safe arena if jobsystem provided.
        Model(core::MemoryArenaBase* arena, core::V2::JobSystem* pJobSys = nullptr);
        ~Model() = default;

        void Clear(void);

        bool LoadRawModel(core::Path<char>& path);
        bool LoadRawModel(const core::Array<uint8_t>& data);

        bool SaveRawModel(core::Path<char>& path);
        // save to a buffer
        bool SaveRawModel(core::Array<uint8_t>& data);

        size_t totalMeshes(void) const;
        size_t numLods(void) const;
        bool hasColMeshes(void) const;

        size_t getNumBones(void) const;
        const char* getBoneName(size_t idx) const;
        const Quatf getBoneAngle(size_t idx) const;
        const Vec3f getBonePos(size_t idx) const;

        const Lod& getLod(size_t idx) const;

        core::UnitOfMeasure::Enum getUnits(void) const;

    private:
        bool ParseRawModel(core::XLexer& lex);
        bool ReadBones(core::XLexer& lex, int32_t numBones);
        bool ReadLods(core::XLexer& lex, int32_t numLods);
        bool ReadMesh(core::XLexer& lex, Mesh& mesh);
        static bool ReadMaterial(core::XLexer& lex, Material& mat);
        static bool ReadMaterialCol(core::XLexer& lex, const char* pName, Color& col);

        static bool ReadheaderToken(core::XLexer& lex, const char* pName, core::UnitOfMeasure::Enum& units, bool optional);
        static bool ReadheaderToken(core::XLexer& lex, const char* pName, int32_t& valOut);
        static bool ReadheaderToken(core::XLexer& lex, const char* pName, uint32_t& valOut);

        bool SaveRawModel_Int(ModelDataStrArr& dataArr) const;
        bool WriteBones(ModelDataStrArr& arr) const;
        bool WriteLods(ModelDataStrArr& arr) const;
        bool WriteMeshes(ModelDataStrArr& arr, const Lod& lod) const;
        // static as job version makes use of it, to reduce code duplication.
        static bool WriteMesh(ModelDataStrArr& arr, const Mesh& mesh, core::MemoryArenaBase* arena);
        bool WriteMaterial(ModelDataStrArr& arr, const Material& mat) const;

    protected:
        static bool isColisionMesh(const RawModel::Mesh::NameString& name, ColMeshType::Enum* pType = nullptr);

    private:
        struct MeshWriteData
        {
            X_INLINE MeshWriteData(core::MemoryArenaBase* arena_) :
                arena(arena_),
                data(arena),
                pMesh(nullptr),
                pJob(nullptr)
            {
            }

            core::MemoryArenaBase* arena;
            ModelDataStrArr data;
            const RawModel::Mesh* pMesh;
            core::V2::Job* pJob;
        };

        typedef core::Array<MeshWriteData> MeshWriteDataArr;

        static void WriteMeshDataJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pJobData);

    protected:
        core::V2::JobSystem* pJobSys_;
        core::MemoryArenaBase* arena_;

        BoneArr bones_;
        LodArr lods_;

        core::UnitOfMeasure::Enum unitOfMeasure_;
        bool hasColisionMeshes_;
    };

} // namespace RawModel

X_NAMESPACE_END