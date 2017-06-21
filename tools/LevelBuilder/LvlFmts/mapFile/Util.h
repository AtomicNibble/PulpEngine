#pragma once


X_NAMESPACE_BEGIN(lvl)

namespace mapFile
{

	void TextureAxisFromPlane(const Vec3f& normal, Vec3f& a2, Vec3f& a3);
	void QuakeTextureVecs(const Planef& plane, Vec2f shift, float rotate, Vec2f scale, Vec4f mappingVecs[2]);


} // namespae mapFile

X_NAMESPACE_END
