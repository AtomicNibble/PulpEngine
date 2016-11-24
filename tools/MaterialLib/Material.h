#pragma once

#ifndef X_MATERIAL_H_
#define X_MATERIAL_H_

#include <IMaterial.h>
#include <IRender.h>
#include <IRenderCommands.h>

X_NAMESPACE_DECLARE(render,

	namespace Commands {
		struct ResourceStateBase;
	} // namespace Commanfs.

	namespace shader {
		struct IShader;
	} // namespace shader

);


X_NAMESPACE_BEGIN(engine)

class Material
{
public:
	X_INLINE Material();
	~Material() = default;

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
	X_INLINE void setStateDesc(render::StateDesc& stateDesc);
	X_INLINE void setStateHandle(render::StateHandle handle);
	X_INLINE void setVariableState(render::Commands::ResourceStateBase* pState);

	// flag helpers.
	X_INLINE bool isDrawn(void) const;
	X_INLINE bool isLoaded(void) const;

	X_INLINE const core::string& getName(void) const;
	X_INLINE MaterialFlags getFlags(void) const;
	X_INLINE MaterialSurType::Enum getSurfaceType(void) const;
	X_INLINE MaterialCoverage::Enum getCoverage(void) const;
	X_INLINE MaterialPolygonOffset::Enum getPolyOffsetType(void) const;
	X_INLINE MaterialMountType::Enum getMountType(void) const;
	X_INLINE MaterialCat::Enum getCat(void) const;
	X_INLINE const render::StateDesc& getStateDesc(void) const;
	X_INLINE render::StateHandle getStateHandle(void) const;
	X_INLINE render::Commands::ResourceStateBase* getVariableState(void) const;


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
	uint8_t numCBs_; // the number of const buffers this material requires.
	uint8_t __pad[2];


	// we store things like blend. cullType etc in the form required for passing to render system.
	render::StateDesc stateDesc_;

	// stuff if we have valid render device.
	render::StateHandle stateHandle_; // the pipeline state required for this material.
	render::Commands::ResourceStateBase* pVariableState_;
	render::shader::IShader* pShader_;
};

X_NAMESPACE_END

#include "Material.inl"

#endif // X_MATERIAL_H_