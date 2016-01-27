#pragma once


#ifndef X_CONVETER_MODULE_I_H_
#define X_CONVETER_MODULE_I_H_

#include <Extension\IPotatoClass.h>


struct IConverterModule : public IPotatoClass
{
	virtual const char* GetName(void) X_ABSTRACT;

	virtual bool Initialize(void) X_ABSTRACT;
	virtual bool ShutDown(void) X_ABSTRACT;
};


#endif // !X_CONVETER_MODULE_I_H_