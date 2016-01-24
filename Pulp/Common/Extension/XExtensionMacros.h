#pragma once

#ifndef _X_POTATO_EXTENSION_MACROS_H_
#define _X_POTATO_EXTENSION_MACROS_H_

#define _STDINT // fuck standard types!
#include <memory>

#include "IPotatoClass.h"
#include "IPotatoFactory.h"

#include "FactoryRegNode.h"


#define X_POTATO_FACTORY_BEGIN(cname)\
private:\
class XFactory : public IPotatoFactory \
	{\
	public:\
		virtual const char* GetName() const\
		{\
			return cname; \
		}

#define X_POTATO_FACTORY_CREATECLASSINSTANCE(classname) \
	public:\
	virtual std::shared_ptr<IPotatoClass> CreateInstance() const\
		{\
		std::shared_ptr<classname> p(new classname); \
		return p; \
		}

#define X_POTATO_FACTORY_END()\
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

#define X_POTATO_ENFORCE_CRYFACTORY_USAGE(classname)\
public:\
	static std::shared_ptr<classname> CreateInstance()\
	{\
	std::shared_ptr<IPotatoClass> p = XFactory::Access().CreateInstance(); \
	return std::shared_ptr<classname>(*static_cast<std::shared_ptr<classname>*>(static_cast<void*>(&p))); \
	}\
	\
public:\
	classname(); \
	virtual ~classname();


#define X_POTATO_IMPLEMENT_POTATOCLASS()\
public:\
	virtual IPotatoFactory* GetFactory() const\
	{\
		return &XFactory::Access(); \
	}



#define X_POTATO_GENERATE_SINGLETONCLASS(classname, cname)\
	X_POTATO_FACTORY_BEGIN(cname)\
	X_POTATO_FACTORY_CREATECLASSINSTANCE(classname)\
	X_POTATO_FACTORY_END()\
	X_POTATO_IMPLEMENT_POTATOCLASS()\
	X_POTATO_ENFORCE_CRYFACTORY_USAGE(classname)


#define X_POTATO_REGISTER_CLASS(classname)\
	classname::XFactory classname::XFactory::factory__;


#endif // !_X_POTATO_EXTENSION_MACROS_H_
