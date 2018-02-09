#pragma once


X_NAMESPACE_BEGIN(assman)

namespace Util
{

	bool loadTexture(const core::Array<uint8_t>& imgData, QImage& dst, 
		core::MemoryArenaBase* swap, const QString& suffix = QString());

	bool loadTexture(const core::Array<uint8_t>& imgData, QPixmap& dst,
		core::MemoryArenaBase* swap, const QString& suffix = QString());

}

X_NAMESPACE_END