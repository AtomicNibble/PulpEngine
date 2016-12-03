#pragma once


#ifndef _X_POTATO_FACTORY_I_H_
#define _X_POTATO_FACTORY_I_H_


struct PotatoGUID;
struct IPotatoUnknown;

struct IPotatoFactory
{
protected:
	virtual ~IPotatoFactory() {};
public:
	virtual const char* GetName(void) const X_ABSTRACT;
	virtual const PotatoGUID& GetGUID(void) const X_ABSTRACT;
	virtual bool ClassSupports(const PotatoGUID& guid) const X_ABSTRACT;
	virtual IPotatoUnknown* CreateInstance(void) const X_ABSTRACT;


};

#endif // !_X_POTATO_FACTORY_I_H_
