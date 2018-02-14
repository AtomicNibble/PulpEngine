#include "stdafx.h"
#include "Material.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(engine)

void Material::assignProps(const Material& oth)
{
	flags_ = oth.flags_;
	surfaceType_ = oth.surfaceType_;
	coverage_ = oth.coverage_;
	polyOffsetType_ = oth.polyOffsetType_;
	mountType_ = oth.mountType_;
	
	usage_ = oth.usage_;
	cat_ = oth.cat_;
	status_ = oth.status_;
	
	tiling_ = oth.tiling_;
	atlas_ = oth.atlas_;

	pTechDefState_ = oth.pTechDefState_;
	techs_ = oth.techs_;
	textures_ = oth.textures_;
}

void Material::assignProps(const MaterialHeader& hdr)
{
	X_ASSERT(hdr.isValid(), "Header must be valid")(hdr.isValid());

	flags_ = hdr.flags;
	surfaceType_ = hdr.surfaceType;
	coverage_ = hdr.coverage;
	mountType_ = hdr.mountType;
	usage_ = hdr.usage;
	cat_ = hdr.cat;
	
	tiling_ = hdr.tiling;
	atlas_ = hdr.atlas;
}


X_NAMESPACE_END
