#pragma once


#ifndef X_TEXTURE_LOADER_CI_H_
#define X_TEXTURE_LOADER_CI_H_

#include "ITexture.h"
#include <String\Path.h>

X_NAMESPACE_BEGIN(texture)

namespace CI
{

	class XTexLoaderCI : public ITextureLoader
	{
	public:
		XTexLoaderCI();
		~XTexLoaderCI();

		// ITextureLoader

		virtual bool canLoadFile(const core::Path<char>& path) const X_OVERRIDE;
		virtual XTextureFile* loadTexture(core::XFile* file) X_OVERRIDE;

		// ~ITextureLoader

	private:

	};

} // namespace CI


X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_CI_H_