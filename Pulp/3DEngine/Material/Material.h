#pragma once

#ifndef X_MATERIAL_OLD_H_
#define X_MATERIAL_OLD_H_

#include "IShader.h"
#include "IMaterial.h"

#include "String\StrRef.h"
#include "Assets\AssertContainer.h"

#include "..\EngineBase.h"


X_NAMESPACE_DECLARE(core,
	namespace xml {
		namespace rapidxml {
			template<class Ch> class xml_node;
		}
	}
)

X_NAMESPACE_BEGIN(engine)

class XMaterialManager;

class XMaterial : public IMaterial, public XEngineBase
{
public:
	XMaterial();
	~XMaterial();

	X_INLINE const int32_t getID(void) const;
	X_INLINE void setID(int32_t id);

	virtual const core::string& getName(void) const X_OVERRIDE { return matName_; };
	virtual void setName(const char* pName) X_OVERRIDE;

	virtual const MaterialFlags getFlags() const X_OVERRIDE;
	virtual void setFlags(const MaterialFlags flags) X_OVERRIDE;

	virtual MaterialSurType::Enum getSurfaceType() const X_OVERRIDE;
	virtual void setSurfaceType(MaterialSurType::Enum type) X_OVERRIDE;

	virtual MaterialCullType::Enum getCullType() const X_OVERRIDE;
	virtual void setCullType(MaterialCullType::Enum type) X_OVERRIDE;

	virtual MaterialTexRepeat::Enum getTexRepeat(void) const X_OVERRIDE;
	virtual void setTexRepeat(MaterialTexRepeat::Enum texRepeat) X_OVERRIDE;

	virtual MaterialPolygonOffset::Enum getPolyOffsetType(void) const X_OVERRIDE;
	virtual void setPolyOffsetType(MaterialPolygonOffset::Enum polyOffsetType) X_OVERRIDE;

	virtual MaterialFilterType::Enum getFilterType(void) const X_OVERRIDE;
	virtual void setFilterType(MaterialFilterType::Enum filterType) X_OVERRIDE;

//	virtual MaterialType::Enum getType(void) const X_OVERRIDE;
//	virtual void setType(MaterialType::Enum type) X_OVERRIDE;

	virtual MaterialCoverage::Enum getCoverage(void) const X_OVERRIDE;
	virtual void setCoverage(MaterialCoverage::Enum coverage) X_OVERRIDE;

//	virtual void setShaderItem(shader::XShaderItem& item) X_OVERRIDE;
//	virtual shader::XShaderItem& getShaderItem(void) X_OVERRIDE{ return shaderItem_; }

	virtual bool isDefault() const X_OVERRIDE;

protected:
	X_NO_COPY(XMaterial);
	X_NO_ASSIGN(XMaterial);

	int32_t id_;

	core::string			matName_;
	MaterialFlags			flags_;
	MaterialSurType::Enum	MatSurfaceType_;
	MaterialCullType::Enum	CullType_;

	MaterialTexRepeat::Enum texRepeat_;
	MaterialPolygonOffset::Enum polyOffsetType_;
	MaterialFilterType::Enum filterType_;
//	MaterialType::Enum		MatType_;
	MaterialCoverage::Enum  coverage_;

//	shader::XShaderItem     shaderItem_;

};


X_NAMESPACE_END

#include "Material.inl"


#endif // X_MATERIAL_OLD_H_