#pragma once

#include <Containers\Array.h>

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

	class Vert
	{
		typedef core::FixedArray<Bind, 12> BindsArr;
	public:
		Vert() = default;
		~Vert() = default;

		Vec3f pos_;
		Vec3f normal_;
		Color col_;
		Vec2f uv_;

		BindsArr binds_;
	};

	typedef int32_t Index;
	typedef Vec3<Index> Face;


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
		typedef core::Array<Vert> VertsArr;
		typedef core::Array<Face> FaceArr;

	public:
		Mesh(core::MemoryArenaBase* arena);
		~Mesh() = default;

		VertsArr verts_;
		FaceArr face_;
		Material material_;
	};

	class Lod
	{
		typedef core::Array<Mesh> MeshArr;

	public:
		Lod(core::MemoryArenaBase* arena);
		~Lod() = default;

		float32_t distance_;
		MeshArr meshes_;
	};

} // namespace RawModel


X_NAMESPACE_END