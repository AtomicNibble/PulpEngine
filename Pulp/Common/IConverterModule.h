#pragma once


#ifndef X_CONVETER_MODULE_I_H_
#define X_CONVETER_MODULE_I_H_

#include <Extension\IPotatoClass.h>
#include <String\CmdArgs.h>

X_NAMESPACE_DECLARE(anim,
struct 	IAnimLib
)

X_NAMESPACE_DECLARE(model,
struct IModelLib
	)


struct IConverter;
struct IConverterModule : public IPotatoClass
{
	virtual const char* GetName(void) X_ABSTRACT;

	virtual IConverter* Initialize(void) X_ABSTRACT;
	virtual bool ShutDown(IConverter* pCon) X_ABSTRACT;
};


struct IConverter
{
	typedef core::CmdArgs<4096, wchar_t> ConvertArgs;

	virtual ~IConverter() {}

	virtual bool Convert(ConvertArgs& args) X_ABSTRACT;
};




#endif // !X_CONVETER_MODULE_I_H_