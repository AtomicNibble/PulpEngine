#pragma once


#ifndef X_TEXTURE_LOADER_PNG_H_
#define X_TEXTURE_LOADER_PNG_H_

#include "ITexture.h"
#include <String\Path.h>

X_NAMESPACE_BEGIN(texture)

namespace PNG
{

	class XTexLoaderPNG : public ITextureLoader
	{
	public:
		XTexLoaderPNG();
		~XTexLoaderPNG();

		// ITextureLoader

		virtual bool canLoadFile(const core::Path& path) const X_OVERRIDE;
		virtual XTextureFile* loadTexture(core::XFile* file) X_OVERRIDE;

		// ~ITextureLoader

	private:

	};

} // namespace PNG


X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_PNG_H_