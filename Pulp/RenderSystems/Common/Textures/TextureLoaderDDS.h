#pragma once


#ifndef X_TEXTURE_LOADER_DDS_H_
#define X_TEXTURE_LOADER_DDS_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace DDS
{

	class XTexLoaderDDS : public ITextureLoader
	{
	public:
		XTexLoaderDDS();
		~XTexLoaderDDS();

		// ITextureLoader

		virtual bool canLoadFile(const core::Path<char>& path) const X_OVERRIDE;
		virtual XTextureFile* loadTexture(core::XFile* file) X_OVERRIDE;

		// ~ITextureLoader


	private:

	};

} // namespace DDS


X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_DDS_H_