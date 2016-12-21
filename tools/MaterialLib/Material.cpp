#include "stdafx.h"
#include "Material.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(engine)

#if 1


void Material::assignProps(const Material& oth)
{
	flags_ = oth.flags_;
	surfaceType_ = oth.surfaceType_;
	coverage_ = oth.coverage_;
	polyOffsetType_ = oth.polyOffsetType_;
	mountType_ = oth.mountType_;
	
	usage_ = oth.usage_;
	cat_ = oth.cat_;
	
	tiling_ = oth.tiling_;
	numTextures_ = oth.numTextures_;
	pTechDefState_ = oth.pTechDefState_;
	techs_ = oth.techs_;
	textures_ = oth.textures_;
}


#else

Material::Material()
{

}

bool Material::load(const core::Array<uint8_t>& fileData)
{
	core::XFileFixedBuf file(fileData.begin(), fileData.end());

	return load(&file);
}

bool Material::load(core::XFile* pFile)
{
	MaterialHeader hdr;

	if (pFile->readObj(hdr) != sizeof(hdr)) {
		return false;
	}

	if (hdr.isValid()) {
		return false;
	}

	// this be like some crazy material shieet.
	cat_ = hdr.cat;
	surfaceType_ = hdr.surfaceType;
	usage_ = hdr.usage;
	cullType_ = hdr.cullType;
	polyOffsetType_ = hdr.polyOffsetType;
	coverage_ = hdr.coverage;
	srcBlendColor_ = hdr.srcBlendColor;
	dstBlendColor_ = hdr.dstBlendColor;
	srcBlendAlpha_ = hdr.srcBlendAlpha;
	dstBlendAlpha_ = hdr.dstBlendAlpha;
	mountType_ = hdr.mountType;
	depthTest_ = hdr.depthTest;
	stateFlags_ = hdr.stateFlags;
	flags_ = hdr.flags;
	tiling_ = hdr.tiling;
	diffuse_ = hdr.diffuse;
	specular_ = hdr.specular;
	emissive_ = hdr.emissive;
	shineness_ = hdr.shineness;
	opacity_ = hdr.opacity;


	return true;
}

void Material::assignProps(const Material& oth)
{
	cat_ = oth.cat_;
	surfaceType_ = oth.surfaceType_;
	usage_ = oth.usage_;
	cullType_ = oth.cullType_;
	polyOffsetType_ = oth.polyOffsetType_;
	coverage_ = oth.coverage_;
	srcBlendColor_ = oth.srcBlendColor_;
	dstBlendColor_ = oth.dstBlendColor_;
	srcBlendAlpha_ = oth.srcBlendAlpha_;
	dstBlendAlpha_ = oth.dstBlendAlpha_;
	mountType_ = oth.mountType_;
	depthTest_ = oth.depthTest_;
	stateFlags_ = oth.stateFlags_;
	flags_ = oth.flags_;
	tiling_ = oth.tiling_;
	diffuse_ = oth.diffuse_;
	specular_ = oth.specular_;
	emissive_ = oth.emissive_;
	shineness_ = oth.shineness_;
	opacity_ = oth.opacity_;
}

#endif

X_NAMESPACE_END
