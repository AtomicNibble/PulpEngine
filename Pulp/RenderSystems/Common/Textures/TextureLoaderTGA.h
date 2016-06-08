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
		static bool isValidImageType(uint32_t type);
		static bool isColorMap(uint32_t type);
		static bool isBGR(uint32_t type);
		static bool isMono(uint32_t type);
		static bool isRle(uint32_t type);

		static bool isRightToLeft(uint32_t descriptor);
		static bool isTopToBottom(uint32_t descriptor);


	};

} // namespace TGA


X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_TGA_H_