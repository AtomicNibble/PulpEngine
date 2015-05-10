#include "stdafx.h"
#include "Material.h"


X_NAMESPACE_BEGIN(engine)

bool MaterialHeader::isValid(void) const
{
	if (version != MTL_B_VERSION) {
		X_ERROR("Mtl", "material version is invalid. FileVer: %i RequiredVer: %i",
			version, MTL_B_VERSION);
	}

	return version == MTL_B_VERSION &&
		fourCC == MTL_B_FOURCC;
}



XMaterial::XMaterial() 
{
	MatSurfaceType_;
	CullType_ = MaterialCullType::BACK_SIDED;

}

XMaterial::~XMaterial()
{
	ShutDown();
}

void XMaterial::ShutDown(void)
{
	core::SafeRelease(shaderItem_.pShader_);
	core::SafeRelease(shaderItem_.pResources_);

	// free my nipples!
	((XMaterialManager*)getMaterialManager())->unregister(this);

}

// XBaseAsset
const int XMaterial::release()
{
	const int ref = XBaseAsset::release();
	if (ref == 0)
		X_DELETE(this, g_3dEngineArena);
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

MaterialType::Enum XMaterial::getType(void) const
{
	return MatType_;
}

void XMaterial::setType(MaterialType::Enum type)
{
	MatType_ = type;
}


void XMaterial::setShaderItem(shader::XShaderItem& item)
{
	core::SafeRelease(shaderItem_.pShader_);
	core::SafeRelease(shaderItem_.pResources_);

	shaderItem_ = item;
}


bool XMaterial::isDefault() const
{
	return getMaterialManager()->getDefaultMaterial() == this;
}


X_NAMESPACE_END
