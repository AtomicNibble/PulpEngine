#pragma once


#ifndef X_TEXTURE_LOADER_JPG_H_
#define X_TEXTURE_LOADER_JPG_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace JPG
{

	class IMGLIB_EXPORT XTexLoaderJPG : public ITextureFmt
	{
	public:
		XTexLoaderJPG();
		~XTexLoaderJPG();

		static bool isValidData(const DataVec& fileData);

		// ITextureFmt
		virtual const char* getExtension(void) const X_OVERRIDE;
		virtual ImgFileFormat::Enum getSrcFmt(void) const X_OVERRIDE;
		virtual bool canLoadFile(const core::Path<char>& path) const X_OVERRIDE;
		virtual bool canLoadFile(const DataVec& fileData) const X_OVERRIDE;
		virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_OVERRIDE;

		// ~ITextureFmt

	private:

	};

} // namespace JPG


X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_JPG_H_