#pragma once


#ifndef X_TEXTURE_LOADER_TGA_H_
#define X_TEXTURE_LOADER_TGA_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace TGA
{

	class XTexLoaderTGA : public ITextureLoader
	{
	public:
		XTexLoaderTGA();
		~XTexLoaderTGA();

		// ITextureLoader

		virtual bool canLoadFile(const core::Path<char>& path) const X_OVERRIDE;
		virtual XTextureFile* loadTexture(core::XFile* file) X_OVERRIDE;

		// ~ITextureLoader


	private:

	};

} // namespace TGA


X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_TGA_H_