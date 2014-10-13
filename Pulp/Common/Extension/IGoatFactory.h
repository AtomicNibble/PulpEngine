#pragma once


#ifndef _X_GOAT_FACTORY_I_H_
#define _X_GOAT_FACTORY_I_H_



struct IGoatClass;

struct IGoatFactory
{
protected:
	virtual ~IGoatFactory() {};
public:
	virtual const char* GetName() const X_ABSTRACT;
	virtual std::shared_ptr<IGoatClass> CreateInstance() const X_ABSTRACT;


};

#endif // !_X_GOAT_FACTORY_I_H_
