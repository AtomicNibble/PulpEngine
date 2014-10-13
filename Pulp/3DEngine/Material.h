#pragma once

#ifndef X_MATERIAL_H_
#define X_MATERIAL_H_

#include "IShader.h"
#include "IMaterial.h"

#include "String\StrRef.h"
#include "Assets\AssertContainer.h"

#include "EngineBase.h"

X_NAMESPACE_BEGIN(engine)

class XMaterialManager;

class XMaterial : public IMaterial, public core::XBaseAsset, public XEngineBase
{
public:
	XMaterial();
	~XMaterial();

	void ShutDown(void);

	// XBaseAsset
	virtual const int release() X_OVERRIDE;
	// ~XBaseAsset

	virtual const char* getName() X_OVERRIDE { return MatName_; };
	virtual void setName(const char* pName) X_OVERRIDE;

	virtual const MaterialFlags getFlags() const X_OVERRIDE;
	virtual void setFlags(const MaterialFlags flags) X_OVERRIDE;

	virtual MaterialSurType::Enum getSurfaceType() const X_OVERRIDE;
	virtual void setSurfaceType(MaterialSurType::Enum type) X_OVERRIDE;

	virtual MaterialCullType::Enum getCullType() const X_OVERRIDE;
	virtual void setCullType(MaterialCullType::Enum type) X_OVERRIDE;

	virtual void setShaderItem(shader::XShaderItem& item) X_OVERRIDE;
	virtual shader::XShaderItem& getShaderItem(void) X_OVERRIDE{ return shaderItem_; }

	virtual bool isDefault() const X_OVERRIDE;



protected:
	friend class XMaterialManager;

	X_NO_COPY(XMaterial);
	X_NO_ASSIGN(XMaterial);

	core::string			MatName_;
	MaterialFlags			flags_;
	MaterialSurType::Enum	MatSurfaceType_;
	MaterialCullType::Enum	CullType_;

	shader::XShaderItem     shaderItem_;

};


X_NAMESPACE_END



#endif // X_MATERIAL_H_