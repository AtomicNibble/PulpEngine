#include "stdafx.h"
#include "ImageLoader.h"


#include <../ImgLib/ImgLib.h>

X_LINK_ENGINE_LIB("imgLib");


X_NAMESPACE_BEGIN(assman)

namespace Util
{

	bool loadTexture(const core::Array<uint8_t>& imgData, QImage& img, 
		core::MemoryArenaBase* swap, const QString& suffix)
	{
		const char* pFormat = nullptr;

		if (suffix.compare("dds", Qt::CaseInsensitive) == 0) {
			pFormat = "dds";
		}
		else if (suffix.compare("tga", Qt::CaseInsensitive) == 0) {
			pFormat = "tga";
		}
		else if (suffix.compare("png", Qt::CaseInsensitive) == 0) {
			pFormat = "png";
		}
		else if (suffix.compare("jpg", Qt::CaseInsensitive) == 0) {
			pFormat = "jpg";
		}

		if (img.loadFromData(imgData.data(), static_cast<uint32_t>(imgData.size()), pFormat)) {
			return true;
		}

		auto fmt = texture::Util::resolveSrcfmt(imgData);
		if (fmt == texture::ImgFileFormat::UNKNOWN) {
			return false;
		}

		texture::XTextureFile imgFile(swap);

		if (!texture::Util::loadImage(swap, imgData, fmt, imgFile)) {
			return false;
		}

		// swap bgr.
		if (texture::Util::isBGR(imgFile.getFormat())) {
			if (!texture::Util::bgrToRgb(imgFile, swap)) {
				return false;
			}
		}

		QImage::Format qtFmt = QImage::Format_Invalid;
		if (imgFile.getFormat() == texture::Texturefmt::R8G8B8A8) {
			qtFmt = QImage::Format_RGBA8888;
		}
		else if (imgFile.getFormat() == texture::Texturefmt::R8G8B8) {
			qtFmt = QImage::Format_RGB888;
		}
		else if (imgFile.getFormat() == texture::Texturefmt::A8) {
			qtFmt = QImage::Format_Alpha8;
		}
		else {
			X_ASSERT_NOT_IMPLEMENTED();
			return false;
		}

		img = QImage(imgFile.getWidth(), imgFile.getHeight(), qtFmt);
		const auto bytesPerRow = img.bytesPerLine();

		if (bytesPerRow != imgFile.getLevelRowbytes(0)) {
			return false;
		}

		const auto* pSrcData = imgFile.getLevel(0, 0);

		for (int32_t y = 0; y < img.height(); y++)
		{
			// need to flip each row.
			const auto* pSrcRow = pSrcData + (y * bytesPerRow);
			std::memcpy(img.scanLine(y), pSrcRow, bytesPerRow);
		}

		return true;
	}


	bool loadTexture(const core::Array<uint8_t>& imgData, QPixmap& pixmap,
		core::MemoryArenaBase* swap, const QString& suffix)
	{
		QImage img;

		if (!loadTexture(imgData, img, swap, suffix)) {
			return false;
		}

		pixmap = QPixmap::fromImage(std::move(img));
		return true;
	}

}

X_NAMESPACE_END