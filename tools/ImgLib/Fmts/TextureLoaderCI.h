#pragma once


#ifndef X_TEXTURE_LOADER_CI_H_
#define X_TEXTURE_LOADER_CI_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace CI
{

	class XTexLoaderCI : public ITextureFmt
	{
	public:
		XTexLoaderCI();
		~XTexLoaderCI();


		// ITextureFmt

		virtual bool canLoadFile(const core::Path<char>& path) const X_OVERRIDE;
		virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_OVERRIDE;

		// ~ITextureFmt

	private:

	};

} // namespace CI


X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_CI_H_