#pragma once


#ifndef _X_POTATO_CLASS_I_H_
#define _X_POTATO_CLASS_I_H_

struct IPotatoFactory;


struct IPotatoClass
{
	virtual ~IPotatoClass() {}

	virtual IPotatoFactory* GetFactory() const X_ABSTRACT;

};


#endif // !_X_POTATO_CLASS_I_H_
