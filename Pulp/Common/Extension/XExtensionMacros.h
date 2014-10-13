#pragma once

#ifndef _X_GOAT_EXTENSION_MACROS_H_
#define _X_GOAT_EXTENSION_MACROS_H_

#define _STDINT // fuck standard types!
#include <memory>

#include "IGoatClass.h"
#include "IGoatFactory.h"

#include "FactoryRegNode.h"


#define X_GOAT_FACTORY_BEGIN(cname)\
private:\
class XFactory : public IGoatFactory \
	{\
	public:\
		virtual const char* GetName() const\
		{\
			return cname; \
		}

#define X_GOAT_FACTORY_CREATECLASSINSTANCE(classname) \
	public:\
	virtual std::shared_ptr<IGoatClass> CreateInstance() const\
		{\
		std::shared_ptr<classname> p(new classname); \
		return p; \
		}

#define X_GOAT_FACTORY_END()\
	public:\
	static XFactory& Access(){\
			return factory__; \
		}\
		\
	private:\
		XFactory() : \
		registerFactory_() \
		{ \
			new(&registerFactory_) XRegFactoryNode(this); \
		}\
		\
		XFactory(const XFactory&); \
		XFactory& operator =(const XFactory&); \
		\
	private:\
		static XFactory factory__; \
	private: \
		XRegFactoryNode registerFactory_; \
	};

#define X_GOAT_ENFORCE_CRYFACTORY_USAGE(classname)\
public:\
	static std::shared_ptr<classname> CreateInstance()\
	{\
	std::shared_ptr<IGoatClass> p = XFactory::Access().CreateInstance(); \
	return std::shared_ptr<classname>(*static_cast<std::shared_ptr<classname>*>(static_cast<void*>(&p))); \
	}\
	\
public:\
	classname(); \
	virtual ~classname();


#define X_GOAT_IMPLEMENT_GOATCLASS()\
public:\
	virtual IGoatFactory* GetFactory() const\
	{\
		return &XFactory::Access(); \
	}



#define X_GOAT_GENERATE_SINGLETONCLASS(classname, cname)\
	X_GOAT_FACTORY_BEGIN(cname)\
	X_GOAT_FACTORY_CREATECLASSINSTANCE(classname)\
	X_GOAT_FACTORY_END()\
	X_GOAT_IMPLEMENT_GOATCLASS()\
	X_GOAT_ENFORCE_CRYFACTORY_USAGE(classname)


#define X_GOAT_REGISTER_CLASS(classname)\
	classname::XFactory classname::XFactory::factory__;


#endif // !_X_GOAT_EXTENSION_MACROS_H_
