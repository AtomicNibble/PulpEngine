#pragma once

#ifndef X_MATERIAL_H_
#define X_MATERIAL_H_

#include <IMaterial.h>
#include <IRender.h>
#include <IRenderCommands.h>
#include <CBuffer.h>
#include <IAsyncLoad.h>

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


namespace techset
{

	class TechSetDef;

} // namespace techset

class Texture;
class TechDef;
class TechDefState;


X_DECLARE_ENUM8(TechStatus)(
	NOT_COMPILED,
	COMPILED,
	ERROR
);

struct TechDefPerm
{
	TechStatus::Enum status;
	PermatationFlags permFlags;
	uint8_t _pad[1];
	render::shader::VertexFormat::Enum vertFmt;
	render::StateHandle stateHandle;
	render::shader::IShaderPermatation* pShaderPerm;
	// +  4 more pad in 64bit
};


struct ParamLink
{
	int32_t paramIdx;
	int32_t cbIdx;
	int32_t cbParamIdx;
};

struct MaterialTech
{
	typedef core::Array<render::shader::XCBuffer> CBufferArr;
	typedef core::Array<const render::shader::XCBuffer*> CBufferPtrArr;
	typedef core::Array<ParamLink> ParamLinkArr;

	MaterialTech(core::MemoryArenaBase* arena);
	MaterialTech(const MaterialTech& oth);
	MaterialTech(MaterialTech&& oth);

	MaterialTech& operator=(const MaterialTech& oth);
	MaterialTech& operator=(MaterialTech&& oth);

	// for lookup.
	core::StrHash::Type hashVal;
	TechDefPerm* pPerm; // the tech perm this came from.

	render::Commands::ResourceStateBase* pVariableState;

	CBufferPtrArr cbs;		// all the cbuffers required.
	CBufferArr materialCbs;	// cbuffers with per material params pre filled.
	ParamLinkArr paramLinks;
};


struct MaterialTexture
{
	X_INLINE MaterialTexture() {
		pTexture = nullptr;
	}

	core::string name;
	core::string val;
	render::TextureSlot::Enum texSlot;
	engine::Texture* pTexture;
};

struct MaterialParam
{
	core::string name;
	ParamType::Enum type;
	Vec4f value;
};

struct MaterialSampler
{
	core::string name;
	render::SamplerState sate;
};


class Material
{
public:
	typedef MaterialTech Tech;
	typedef MaterialTexture Texture;
	typedef MaterialParam Param;
	typedef MaterialSampler Sampler;

	typedef core::Array<Tech> TechArr;
	typedef core::Array<Texture> TextureArr;
	typedef core::Array<Param> ParamArr;
	typedef core::Array<Sampler> SamplerArr;

public:
	X_INLINE Material(core::string& name, core::MemoryArenaBase* arena);
	~Material() = default;

	X_INLINE Tech* getTech(core::StrHash hash, render::shader::VertexFormat::Enum vertFmt, PermatationFlags permFlags);
	X_INLINE void addTech(Tech&& tech);

	// assigns the material props but name styas same etc.
	MATLIB_EXPORT void assignProps(const Material& oth);
	MATLIB_EXPORT void assignProps(const MaterialHeader& hdr);

	X_INLINE const int32_t getID(void) const;
	X_INLINE void setID(int32_t id);

	X_INLINE core::LoadStatus::Enum getStatus(void) const;
	X_INLINE bool isLoaded(void) const;
	X_INLINE bool loadFailed(void) const;
	X_INLINE void setStatus(core::LoadStatus::Enum status);
	X_INLINE bool waitForLoad(IMaterialManager* pMatMan);

	X_INLINE void setFlags(MaterialFlags flags);
	X_INLINE void setSurfaceType(MaterialSurType::Enum surfaceType);
	X_INLINE void setCoverage(MaterialCoverage::Enum coverage);
	X_INLINE void setPolyOffsetType(MaterialPolygonOffset::Enum polyOffset);
	X_INLINE void setMountType(MaterialMountType::Enum mt);
	X_INLINE void setCat(MaterialCat::Enum cat);
	X_INLINE void setTechDefState(TechDefState* pTechDefState);

	X_INLINE void setTextures(TextureArr&& textures);
	X_INLINE void setParams(ParamArr&& params);
	X_INLINE void setSamplers(SamplerArr&& samplers);


	// flag helpers.
	X_INLINE bool isDrawn(void) const;
	X_INLINE bool isDefault(void) const;

	X_INLINE const core::string& getName(void) const;
	X_INLINE MaterialFlags getFlags(void) const;
	X_INLINE MaterialSurType::Enum getSurfaceType(void) const;
	X_INLINE MaterialCoverage::Enum getCoverage(void) const;
	X_INLINE MaterialPolygonOffset::Enum getPolyOffsetType(void) const;
	X_INLINE MaterialMountType::Enum getMountType(void) const;
	X_INLINE MaterialCat::Enum getCat(void) const;
	X_INLINE TechDefState* getTechDefState(void) const;

	X_INLINE const ParamArr& getParams(void) const;
	X_INLINE const SamplerArr& getSamplers(void) const;
	X_INLINE const TextureArr& getTextures(void) const;

protected:
	X_NO_COPY(Material);
	X_NO_ASSIGN(Material);

	int32_t id_;

	core::Spinlock techLock_;
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
	core::LoadStatus::Enum status_;

	// used for custom texture repeat.
	// if AUTO_TILING the textures dim's are used.
	Vec2<int16_t> tiling_;

	TechDefState* pTechDefState_;

	TechArr techs_;
	ParamArr params_;
	SamplerArr samplers_;
	TextureArr textures_;
};

X_NAMESPACE_END

#include "Material.inl"

#endif // X_MATERIAL_H_