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
		face_(arena)
	{

	}

	void Mesh::merge(const Mesh& oth)
	{
		size_t numVert = verts_.size();
		size_t numFace = face_.size();
		size_t newVertNum = numVert + oth.verts_.size();
		size_t newFaceNum = numFace + oth.face_.size();

		verts_.resize(newVertNum);
		face_.resize(newVertNum);

		size_t i;
		for (i = 0; i < oth.verts_.size(); i++)
		{
			verts_[numVert + i] = oth.verts_[i];
		}

		for (i = 0; i < oth.face_.size(); i++)
		{
			face_[numFace + i] = oth.face_[i];
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
		meshes_(arena)
	{

	}

	size_t Lod::getSubDataSize(const Flags8<model::StreamType>& streams) const
	{
		size_t size = 0;

		for (auto& mesh : meshes_)
		{

			size += sizeof(model::Face) * mesh.face_.size();
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
			total += mesh.face_.size();
		}

		return total * 3;
	}




} // namespace RawModel

X_NAMESPACE_END