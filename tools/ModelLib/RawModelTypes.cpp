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

	Lod::Lod(core::MemoryArenaBase* arena) :
		meshes_(arena)
	{

	}


} // namespace RawModel

X_NAMESPACE_END