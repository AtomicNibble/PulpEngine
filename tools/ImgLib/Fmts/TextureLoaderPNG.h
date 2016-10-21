#pragma once


#ifndef X_TEXTURE_LOADER_PNG_H_
#define X_TEXTURE_LOADER_PNG_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace PNG
{

	class IMGLIB_EXPORT XTexLoaderPNG : public ITextureFmt
	{
	public:
		XTexLoaderPNG();
		~XTexLoaderPNG();

		// ITextureFmt
		virtual const char* getExtension(void) const X_OVERRIDE;
		virtual bool canLoadFile(const core::Path<char>& path) const X_OVERRIDE;
		virtual bool canLoadFile(const DataVec& fileData) const X_OVERRIDE;
		virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_OVERRIDE;

		// ~ITextureFmt

	private:

	};

} // namespace PNG


X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_PNG_H_