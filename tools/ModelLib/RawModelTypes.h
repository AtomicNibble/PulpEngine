#pragma once

#include <Containers\Array.h>
#include <IModel.h>

X_NAMESPACE_BEGIN(model)

namespace RawModel
{
    class Bone
    {
    public:
        Bone() = default;
        ~Bone() = default;

        core::string name_;
        Vec3f worldPos_;
        Vec3f scale_;
        Matrix33f rotation_;
        int32_t parIndx_;
    };

    class Bind
    {
    public:
        Bind() = default;
        ~Bind() = default;

        int32_t boneIdx_;
        float weight_;
    };

    X_DISABLE_WARNING(4324)

    X_ALIGNED_SYMBOL(class Vert, 64)
    {
    public:
        typedef core::FixedArray<Bind, 12> BindsArr;

    public:
        Vert() = default;
        ~Vert() = default;

        Vec3f pos_;
        BindsArr binds_;
    };

    /*
	X_ALIGNED_SYMBOL(class FullVert, 64)
	{
	public:
		typedef core::FixedArray<Bind, 12> BindsArr;
	public:
		FullVert() = default;
		~FullVert() = default;

		Vec3f pos_;
		Vec3f normal_;
		Vec3f tangent_;
		Vec3f biNormal_;
		Color col_;
		Vec2f uv_;

		BindsArr binds_;
	};
	*/

    typedef uint32_t Index;
    typedef Vec3<Index> Face;

    X_ALIGNED_SYMBOL(class TriVert, 64)
    {
    public:
        TriVert() = default;
        ~TriVert() = default;

        Index index_;
        Vec3f normal_;
        Vec3f tangent_;
        Vec3f biNormal_;
        Color col_;
        Vec2f uv_;
    };

    typedef std::array<TriVert, 3> Tri;

    X_ENABLE_WARNING(4324)

    class Material
    {
    public:
        Material();
        ~Material() = default;

        core::string name_;
        Color col_;
        Color tansparency_;
        Color ambientColor_;
        Color specCol_;
        Color reflectiveCol_;
    };

    class Mesh
    {
    public:
        typedef core::StackString<60> NameString;
        typedef core::Array<Vert> VertsArr;
        typedef core::Array<Tri> TriArr;

    public:
        Mesh(core::MemoryArenaBase* arena);
        ~Mesh() = default;

        void merge(const Mesh& oth);

        const Material& getMaterial(void) const;

    public:
        NameString name_;
        NameString displayName_;

        VertsArr verts_;
        TriArr tris_;
        Material material_;
    };

    class Lod
    {
        typedef core::Array<Mesh> MeshArr;

    public:
        Lod(core::MemoryArenaBase* arena);
        ~Lod() = default;

        size_t numMeshes(void) const;
        size_t totalVerts(void) const;
        size_t totalTris(void) const;

        const Mesh& getMesh(size_t idx) const;

    public:
        MeshArr meshes_;
    };

} // namespace RawModel

X_NAMESPACE_END