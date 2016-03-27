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
		reflectiveCol_(0.f,0.f,0.f,1.f)
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
		for (i = 0; i < oth.verts_.size(); i++)
		{
			verts_[numVert + i] = oth.verts_[i];
		}

		const Face::value_type faceOffset = safe_static_cast<Face::value_type, size_t>(numVert);
		for (i = 0; i < oth.tris_.size(); i++)
		{
			Tri& tri = tris_[numTris + i];
			tri = oth.tris_[i];

			for (size_t x = 0; x < 3; x++)
			{
				TriVert& triVert = tri[x];
				triVert.index_ += faceOffset;
			}
		}
	}

	void Mesh::calBoundingbox(void)
	{
		AABB aabb;

		aabb.clear();

		for (const auto& vert : verts_) {
			aabb.add(vert.pos_);
		}

		boundingBox_ = aabb;
	}

	Lod::Lod(core::MemoryArenaBase* arena) :
		distance_(0.f),
		meshes_(arena)
	{

	}

	size_t Lod::getSubDataSize(const Flags8<model::StreamType>& streams) const
	{
		size_t size = 0;

		for (auto& mesh : meshes_)
		{

			size += sizeof(model::Face) * mesh.tris_.size();
			size += sizeof(model::Vertex) * mesh.verts_.size();

			// streams.
			if (streams.IsSet(model::StreamType::COLOR)) {
				size += sizeof(model::VertexColor) * mesh.verts_.size();
			}
			if (streams.IsSet(model::StreamType::NORMALS)) {
				size += sizeof(model::VertexNormal) * mesh.verts_.size();
			}
			if (streams.IsSet(model::StreamType::TANGENT_BI)) {
				size += sizeof(model::VertexTangentBi) * mesh.verts_.size();
			}

			// bind data
		//	size += safe_static_cast<size_t, size_t>(mesh.CompBinds.dataSizeTotal());
		}

		return size;
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

	size_t Lod::totalIndexs(void) const
	{
		size_t total = 0;

		for (const auto& mesh : meshes_) {
			total += mesh.tris_.size();
		}

		return total * 3;
	}




} // namespace RawModel

X_NAMESPACE_END