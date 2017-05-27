#include "stdafx.h"
#include "Texture.h"

X_NAMESPACE_BEGIN(engine)


Texture::Texture(core::string name, texture::TextureFlags flags) :
	fileName_(name),
	flags_(flags)
{
	dimensions_ = Vec2<uint16_t>::zero();
	datasize_ = 0;
	type_ = texture::TextureType::UNKNOWN;
	format_ = texture::Texturefmt::UNKNOWN;
	numMips_ = 0;
	depth_ = 0;
	numFaces_ = 0;
}

X_NAMESPACE_END