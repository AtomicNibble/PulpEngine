#pragma once

X_NAMESPACE_BEGIN(texture)

namespace Util
{
    // this is D3D specific texture util, anything generic should go in imgLib.

    DXGI_FORMAT DXGIFormatFromTexFmt(Texturefmt::Enum fmt);
    Texturefmt::Enum texFmtFromDXGI(DXGI_FORMAT fmt);

} // namespace Util

X_NAMESPACE_END