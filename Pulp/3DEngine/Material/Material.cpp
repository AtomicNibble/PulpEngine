#include "stdafx.h"
#include "Material.h"




X_NAMESPACE_BEGIN(engine)

#if 0

Material::Material() :
	pVariableState_(nullptr),
	pShader_(nullptr)
{
	id_ = -1;

	surfaceType_ = MaterialSurType::NONE;
	polyOffsetType_ = MaterialPolygonOffset::NONE;
	mountType_ = MaterialMountType::NONE;

	usage_ = MaterialUsage::NONE;
	cat_ = MaterialCat::CODE;
}

Material::~Material()
{

}

#endif

X_NAMESPACE_END
