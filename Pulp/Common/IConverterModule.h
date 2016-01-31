#pragma once


#ifndef X_CONVETER_MODULE_I_H_
#define X_CONVETER_MODULE_I_H_

#include <Extension\IPotatoClass.h>

X_NAMESPACE_DECLARE(anim,
struct 	IAnimLib
)

X_NAMESPACE_DECLARE(model,
struct IModelLib
)


struct ConverterLibs
{
	X_INLINE ConverterLibs() {
		core::zero_this(this);
	}

	anim::IAnimLib* pAnimLib;
	model::IModelLib* pModelLib;
};



struct IConverterModule : public IPotatoClass
{
	virtual const char* GetName(void) X_ABSTRACT;

	virtual bool Initialize(ConverterLibs& libs) X_ABSTRACT;
	virtual bool ShutDown(void) X_ABSTRACT;
};




#endif // !X_CONVETER_MODULE_I_H_