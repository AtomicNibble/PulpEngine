#pragma once

#ifndef X_MATERIAL_H_
#define X_MATERIAL_H_

#include <IMaterial.h>
#include <IRender.h>
#include <IRenderCommands.h>

#include <String\StringHash.h>

X_NAMESPACE_DECLARE(render,

	namespace Commands {
		struct ResourceStateBase;
	} // namespace Commanfs.

	namespace shader {
		struct IShader;
	} // namespace shader

);


X_NAMESPACE_BEGIN(engine)

class TechSetDef;
class TechDefState;
class TechDef;

struct MaterialTech
{
	// for lookup.
	core::StrHash hash;
	render::shader::VertexFormat::Enum vertFmt;

	// for drawing.
	render::StateHandle stateHandle;
	render::Commands::ResourceStateBase* pVariableState;
};

class Material
{
public:
	typedef MaterialTech Tech;

	struct Texture
	{
		texture::TexID texId;
		render::FilterType::Enum filterType;
		render::TexRepeat::Enum texRepeat;
	};

	// god dam name hash even tho just int has a constructor.
//	static_assert(core::compileTime::IsPOD<Tech>::Value, "Tech should be POD");

	typedef core::FixedArray<Texture, MTL_MAX_TEXTURES> FixedTextureArr;

	typedef core::Array<Tech> TechArr;
	typedef core::Array<Texture> TextureArr;

public:
	X_INLINE Material(core::MemoryArenaBase* arena);
	~Material() = default;

	X_INLINE Tech* getTech(core::StrHash hash, render::shader::VertexFormat::Enum vertFmt);
	X_INLINE void addTech(const Tech& tech);

	// assigns the material props but name styas same etc.
	MATLIB_EXPORT void assignProps(const Material& oth);

	X_INLINE const int32_t getID(void) const;
	X_INLINE void setID(int32_t id);

	X_INLINE void setName(const core::string& name);
	X_INLINE void setName(const char* pName);
	X_INLINE void setFlags(MaterialFlags flags);
	X_INLINE void setSurfaceType(MaterialSurType::Enum surfaceType);
	X_INLINE void setCoverage(MaterialCoverage::Enum coverage);
	X_INLINE void setPolyOffsetType(MaterialPolygonOffset::Enum polyOffset);
	X_INLINE void setMountType(MaterialMountType::Enum mt);
	X_INLINE void setCat(MaterialCat::Enum cat);
	X_INLINE void setTechDefState(TechDefState* pTechDefState);

	X_INLINE void setTextures(const FixedTextureArr& texArr);


	// flag helpers.
	X_INLINE bool isDrawn(void) const;
	X_INLINE bool isLoaded(void) const;
	X_INLINE bool isDefault(void) const;

	X_INLINE const core::string& getName(void) const;
	X_INLINE MaterialFlags getFlags(void) const;
	X_INLINE MaterialSurType::Enum getSurfaceType(void) const;
	X_INLINE MaterialCoverage::Enum getCoverage(void) const;
	X_INLINE MaterialPolygonOffset::Enum getPolyOffsetType(void) const;
	X_INLINE MaterialMountType::Enum getMountType(void) const;
	X_INLINE MaterialCat::Enum getCat(void) const;
	X_INLINE TechDefState* getTechDefState(void) const;

protected:
	X_NO_COPY(Material);
	X_NO_ASSIGN(Material);

	int32_t id_;

	core::string name_;

	// 4
	MaterialFlags				flags_;
	// 4
	MaterialSurType::Enum		surfaceType_;
	MaterialCoverage::Enum		coverage_;
	MaterialPolygonOffset::Enum polyOffsetType_;
	MaterialMountType::Enum		mountType_;

	MaterialUsage::Enum usage_;
	MaterialCat::Enum cat_;
	uint8_t _pad[1];

	// used for custom texture repeat.
	// if AUTO_TILING the textures dim's are used.
	Vec2<int16_t> tiling_;

	// 4
	uint8_t numTextures_;
	uint8_t __pad[3];

	TechDefState* pTechDefState_;

	TechArr techs_;
	TextureArr textures_;
	// this is old now, even tho never really used :D
	// since the techs will store states for us.
	// this makes it alot nicer since we will only have like maybe 200 techs max.
	// so that results in only 200 calls to render system to make states.
	// instead of having to ask the render system for every material.
	// we also get the advantage of state livetime is not tied to the materials.
	// so we can load/unload materials without having to re ask for states.
	// this will also potentially make shader hot reloading more easy.
};

X_NAMESPACE_END

#include "Material.inl"

#endif // X_MATERIAL_H_