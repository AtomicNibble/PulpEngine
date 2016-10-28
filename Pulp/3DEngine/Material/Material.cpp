#include "stdafx.h"
#include "Material.h"




X_NAMESPACE_BEGIN(engine)


XMaterial::XMaterial() 
{
	MatSurfaceType_ = MaterialSurType::NONE;
	CullType_ = MaterialCullType::BACK_SIDED;

	texRepeat_ = MaterialTexRepeat::TILE_BOTH;
	polyOffsetType_ = MaterialPolygonOffset::NONE;
	filterType_ = MaterialFilterType::LINEAR_MIP_LINEAR;
//	MatType_ = MaterialType::UNKNOWN;
	coverage_ = MaterialCoverage::BAD;
}

XMaterial::~XMaterial()
{
	ShutDown();
}

void XMaterial::ShutDown(void)
{
	X_ASSERT_NOT_IMPLEMENTED();
//	core::SafeRelease(shaderItem_.pShader_);
//	core::SafeRelease(shaderItem_.pResources_);

	// free my nipples!
	if (getMaterialManager()) {
		static_cast<XMaterialManager*>(getMaterialManager())->unregister(this);
	}
}

// XBaseAsset
const int XMaterial::release()
{
	const int ref = XBaseAsset::release();
	if (ref == 0) {
		X_DELETE(this, g_3dEngineArena);
	}
	return ref;
}
// ~XBaseAsset


void XMaterial::setName(const char* pName)
{
	MatName_ = pName;
}



const MaterialFlags XMaterial::getFlags() const
{
	return flags_;
}

void XMaterial::setFlags(const MaterialFlags flags)
{
	flags_ = flags;
}



MaterialSurType::Enum XMaterial::getSurfaceType() const
{
	return MatSurfaceType_;
}

void XMaterial::setSurfaceType(MaterialSurType::Enum type)
{
	MatSurfaceType_ = type;
}

MaterialCullType::Enum XMaterial::getCullType() const
{
	return CullType_;
}

void XMaterial::setCullType(MaterialCullType::Enum type)
{
	CullType_ = type;
}


MaterialTexRepeat::Enum XMaterial::getTexRepeat(void) const
{
	return texRepeat_;
}

void XMaterial::setTexRepeat(MaterialTexRepeat::Enum texRepeat)
{
	texRepeat_ = texRepeat;
}

MaterialPolygonOffset::Enum XMaterial::getPolyOffsetType(void) const
{
	return polyOffsetType_;
}

void XMaterial::setPolyOffsetType(MaterialPolygonOffset::Enum polyOffsetType)
{
	polyOffsetType_ = polyOffsetType;
}

MaterialFilterType::Enum XMaterial::getFilterType(void) const
{
	return filterType_;
}

void XMaterial::setFilterType(MaterialFilterType::Enum filterType)
{
	filterType_ = filterType;
}

#if 0
MaterialType::Enum XMaterial::getType(void) const
{
	return MatType_;
}

void XMaterial::setType(MaterialType::Enum type)
{
	MatType_ = type;
}
#endif

MaterialCoverage::Enum XMaterial::getCoverage(void) const
{
	return coverage_;
}

void XMaterial::setCoverage(MaterialCoverage::Enum coverage)
{
	coverage_ = coverage;
}

#if 0
void XMaterial::setShaderItem(shader::XShaderItem& item)
{
	core::SafeRelease(shaderItem_.pShader_);
	core::SafeRelease(shaderItem_.pResources_);

	shaderItem_ = item;
}
#endif


bool XMaterial::isDefault() const
{
	return getMaterialManager()->getDefaultMaterial() == this;
}


X_NAMESPACE_END
