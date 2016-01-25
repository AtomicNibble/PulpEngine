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

	Lod::Lod(core::MemoryArenaBase* arena) :
		meshes_(arena)
	{

	}


} // namespace RawModel

X_NAMESPACE_END