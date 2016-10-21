#pragma once


#ifndef X_TEXTURE_LOADER_CI_H_
#define X_TEXTURE_LOADER_CI_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace CI
{

	class IMGLIB_EXPORT XTexLoaderCI : public ITextureFmt
	{
	public:
		XTexLoaderCI();
		~XTexLoaderCI();


		static bool isValidData(const DataVec& fileData);

		// ITextureFmt
		virtual const char* getExtension(void) const X_OVERRIDE;
		virtual ImgFileFormat::Enum getSrcFmt(void) const X_OVERRIDE;
		virtual bool canLoadFile(const core::Path<char>& path) const X_OVERRIDE;
		virtual bool canLoadFile(const DataVec& fileData) const X_OVERRIDE;
		virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_OVERRIDE;

		virtual bool canWrite(void) const X_OVERRIDE { return true; }
		virtual bool saveTexture(core::XFile* file, const XTextureFile& imgFile) X_OVERRIDE;

		// ~ITextureFmt

	private:

	};

} // namespace CI


X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_CI_H_