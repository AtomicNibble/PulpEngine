#include "stdafx.h"
#include "Texture.h"


X_NAMESPACE_BEGIN(texture)


	Texture::Texture(const char* pName, TextureFlags flags) :
		fileName_(pName),
		flags_(flags)
	{
		dimensions_ = Vec2<uint16_t>::zero();
		datasize_ = 0;
		type_ = TextureType::UNKNOWN;
		format_ = Texturefmt::UNKNOWN;
		numMips_ = 0;
		depth_ = 0;
		numFaces_ = 0;
	}


	Texture::~Texture()
	{

	}




X_NAMESPACE_END