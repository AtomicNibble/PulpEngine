#pragma once

#ifndef X_MATERIAL_H_
#define X_MATERIAL_H_

#include <IMaterial.h>

X_NAMESPACE_BEGIN(engine)


class Material
{
public:
	MATLIB_EXPORT Material();

	MATLIB_EXPORT bool load(const core::Array<uint8_t>& fileData);
	MATLIB_EXPORT bool load(core::XFile* pFile);

	// assigns the material props but name styas same etc.
	MATLIB_EXPORT void assignProps(const Material& oth);

	X_INLINE const core::string& getName(void) const;
	X_INLINE void setName(const char* pName);

	// flag helpers.
	X_INLINE bool isDrawn(void) const;
	X_INLINE bool isLoaded(void) const;

	X_INLINE const MaterialFlags getFlags(void) const;
	X_INLINE void setFlags(const MaterialFlags flags);

	X_INLINE MaterialCat::Enum getCat(void) const;
	X_INLINE void setCat(MaterialCat::Enum cat);

	X_INLINE MaterialSurType::Enum getSurfaceType(void) const;
	X_INLINE void setSurfaceType(MaterialSurType::Enum surfaceType);

	X_INLINE MaterialCullType::Enum getCullType(void) const;
	X_INLINE void setCullType(MaterialCullType::Enum cullType);

	X_INLINE MaterialPolygonOffset::Enum getPolyOffsetType(void) const;
	X_INLINE void setPolyOffsetType(MaterialPolygonOffset::Enum polyOffsetType);

	X_INLINE MaterialCoverage::Enum getCoverage(void) const;
	X_INLINE void setCoverage(MaterialCoverage::Enum coverage);

private:
	core::string name_; // ugh.

	uint8_t _pad_;
	uint8_t numTextures_;
	MaterialCat::Enum cat_;
	MaterialSurType::Enum surfaceType_;

	// 4
	MaterialUsage::Enum usage_;
	MaterialCullType::Enum cullType_;
	MaterialPolygonOffset::Enum polyOffsetType_;
	MaterialCoverage::Enum coverage_;

	// 4: blend ops.
	MaterialBlendType::Enum srcBlendColor_;
	MaterialBlendType::Enum dstBlendColor_;
	MaterialBlendType::Enum srcBlendAlpha_;
	MaterialBlendType::Enum dstBlendAlpha_;

	// 4
	MaterialMountType::Enum mountType_;
	StencilFunc::Enum depthTest_;
	MaterialStateFlags stateFlags_;
	bool _pad;

	// 4
	MaterialFlags flags_;

	// used for custom texture repeat.
	// if AUTO_TILING the textures dim's are used.
	Vec2<int16_t> tiling_;

	// 12
	Color8u diffuse_;
	Color8u specular_;
	Color8u emissive_;
	// 8
	float shineness_;
	float opacity_;
};

X_NAMESPACE_END

#include "Material.inl"

#endif // X_MATERIAL_H_