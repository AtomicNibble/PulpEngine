#pragma once


#ifndef _X_GOAT_CLASS_I_H_
#define _X_GOAT_CLASS_I_H_

struct IGoatFactory;


struct IGoatClass
{

	virtual IGoatFactory* GetFactory() const =0;

};


#endif // !_X_GOAT_CLASS_I_H_
