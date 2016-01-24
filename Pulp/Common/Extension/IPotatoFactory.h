#pragma once


#ifndef _X_POTATO_FACTORY_I_H_
#define _X_POTATO_FACTORY_I_H_



struct IPotatoClass;

struct IPotatoFactory
{
protected:
	virtual ~IPotatoFactory() {};
public:
	virtual const char* GetName() const X_ABSTRACT;
	virtual std::shared_ptr<IPotatoClass> CreateInstance() const X_ABSTRACT;


};

#endif // !_X_POTATO_FACTORY_I_H_
