#include "stdafx.h"
#include "RawModelTypes.h"

X_NAMESPACE_BEGIN(model)

namespace RawModel
{
    Material::Material() :
        col_(1.f, 1.f, 1.f, 1.f),
        tansparency_(0.f, 0.f, 0.f, 1.f),
        ambientColor_(0.f, 0.f, 0.f, 1.f),
        specCol_(0.f, 0.f, 0.f, 1.f),
        reflectiveCol_(0.f, 0.f, 0.f, 1.f)
    {
    }

    Mesh::Mesh(core::MemoryArenaBase* arena) :
        verts_(arena),
        tris_(arena)
    {
    }

    void Mesh::merge(const Mesh& oth)
    {
        const size_t numVert = verts_.size();
        const size_t numTris = tris_.size();
        const size_t newVertNum = numVert + oth.verts_.size();
        const size_t newTrisNum = numTris + oth.tris_.size();

        verts_.resize(newVertNum);
        tris_.resize(newTrisNum);

        size_t i;
        for (i = 0; i < oth.verts_.size(); i++) {
            verts_[numVert + i] = oth.verts_[i];
        }

        const Face::value_type faceOffset = safe_static_cast<Face::value_type, size_t>(numVert);
        for (i = 0; i < oth.tris_.size(); i++) {
            Tri& tri = tris_[numTris + i];
            tri = oth.tris_[i];

            for (size_t x = 0; x < 3; x++) {
                TriVert& triVert = tri[x];
                triVert.index_ += faceOffset;
            }
        }
    }

    const Material& Mesh::getMaterial(void) const
    {
        return material_;
    }

    Lod::Lod(core::MemoryArenaBase* arena) :
        meshes_(arena)
    {
    }

    size_t Lod::numMeshes(void) const
    {
        return meshes_.size();
    }

    size_t Lod::totalVerts(void) const
    {
        size_t total = 0;

        for (const auto& mesh : meshes_) {
            total += mesh.verts_.size();
        }

        return total;
    }

    size_t Lod::totalTris(void) const
    {
        size_t total = 0;

        for (const auto& mesh : meshes_) {
            total += mesh.tris_.size();
        }

        return total;
    }

    const Mesh& Lod::getMesh(size_t idx) const
    {
        X_ASSERT(idx < numMeshes(), "Invalid mesh idx")
        (idx, numMeshes());
        return meshes_[idx];
    }

} // namespace RawModel

X_NAMESPACE_END