#pragma once

#ifndef _X_POTATO_EXTENSION_MACROS_H_
#define _X_POTATO_EXTENSION_MACROS_H_

#define _STDINT // fuck standard types!
#include <memory>

#include "TypeList.h"

#include "IPotatoUnknown.h"
#include "IPotatoFactory.h"

#include "FactoryRegNode.h"

#include "GUID.h"


template<class TList>
struct FillIIDs;

template<>
struct FillIIDs<TL::NullType>
{
	static void Op(PotatoGUID*)
	{
	}
};

template<class Head, class Tail>
struct FillIIDs<TL::Typelist<Head, Tail>>
{
	static void Op(PotatoGUID* p)
	{
		*p++ = PotatoIdOf<Head>();
		FillIIDs<Tail>::Op(p);
	}
};


template<typename T>
class XFactory : public IPotatoFactory
{

public:
	XFactory() :
		numIIDs_(0),
		pIIDs_(nullptr),
		registerFactory_()
	{
		static PotatoGUID supportedIIDs[TL::Length < typename T::FullInterfaceList > ::value];
		FillIIDs<typename T::FullInterfaceList>::Op(supportedIIDs);
		pIIDs_ = &supportedIIDs[0];
		numIIDs_ = TL::Length<typename T::FullInterfaceList>::value;

		new(&registerFactory_) XRegFactoryNode(this);
	}

	
	virtual const char* GetName(void) const X_OVERRIDE
	{
		return T::GetSName();
	}

	virtual const PotatoGUID& GetGUID(void) const X_OVERRIDE
	{
		return T::GetSGUID();
	}

	virtual bool ClassSupports(const PotatoGUID& iid) const X_OVERRIDE
	{
		for (size_t i = 0; i < numIIDs_; ++i)
		{
			if (iid == pIIDs_[i]) {
				return true;
			}
		}
		return false;
	}

public:
	virtual IPotatoUnknown* CreateInstance(void) const X_OVERRIDE
	{
		return new T;
	}

private:
	XFactory(const XFactory&); 
	XFactory& operator =(const XFactory&); 

private:
	size_t		numIIDs_;
	PotatoGUID*	pIIDs_;
	XRegFactoryNode registerFactory_; 
};


template<typename T>
class XSingletonFactory : public XFactory<T>
{
public:
	XSingletonFactory() :
		XFactory<T>()
	{
	}

	virtual IPotatoUnknown* CreateInstance(void) const X_OVERRIDE
	{
		static IPotatoUnknown* pInst = XFactory<T>::CreateInstance();
		return pInst;
	}
};


// #define POTATO_FACTORY_DECLARE(implclassname) \
//   private:                                 \
//     static XFactory<implclassname> s_factory;

#define _POTATO_FACTORY_DECLARE_SINGLETON(implclassname) \
  private:                                           \
    friend class XFactory<implclassname>;            \
    static XSingletonFactory<implclassname> s_factory;


#define _POTATO_IMPLEMENT_IPOTATOUNKNOWN()                                        \
  public:                                                                  \
    virtual IPotatoFactory* GetFactory() const X_OVERRIDE                  \
    {                                                                      \
      return &s_factory;                                                   \
    }                                                                      \
                                                                           \
  protected: 

#define _POTATO_ADD_MEMBERS(classname, cname, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5,uuid_6,uuid_7,uuid_8,uuid_9,uuid_10,uuid_11) \
public: \
	static const char* GetSName(void) \
	{\
		return cname; \
	}\
	static const PotatoGUID& GetSGUID(void)  \
	{ \
		static const PotatoGUID id(uuid_1, uuid_2, uuid_3, uuid_4, uuid_5,uuid_6,uuid_7,uuid_8,uuid_9,uuid_10,uuid_11);		\
		return id;  \
	} \
	static std::shared_ptr<classname> CreateInstance(void)\
	{\
		IPotatoUnknown* p = s_factory.CreateInstance(); \
		return std::shared_ptr<classname>(*static_cast<std::shared_ptr<classname>*>(static_cast<void*>(&p))); \
	} \
                                 \
  protected:                     \
    classname();             \
    virtual ~classname();


#define X_POTATO_INTERFACE_BEGIN() \
  private:                   \
    typedef TL::MakeTypelist < IPotatoUnknown

#define X_POTATO_INTERFACE_ADD(iname)                        , iname

#define X_POTATO_INTERFACE_END()                             > ::Result _UserDefinedPartialInterfaceList; \
  protected:                                                                                        \
    typedef TL::NoDuplicates<_UserDefinedPartialInterfaceList>::Result FullInterfaceList;




#define  X_POTATO_INTERFACE_SIMPLE(iname) \
  X_POTATO_INTERFACE_BEGIN()             \
  X_POTATO_INTERFACE_ADD(iname)          \
  X_POTATO_INTERFACE_END()



#define X_POTATO_GENERATE_SINGLETONCLASS(classname, cname, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5,uuid_6,uuid_7,uuid_8,uuid_9,uuid_10,uuid_11)\
	_POTATO_FACTORY_DECLARE_SINGLETON(classname) \
	_POTATO_IMPLEMENT_IPOTATOUNKNOWN() \
	_POTATO_ADD_MEMBERS(classname, cname, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5,uuid_6,uuid_7,uuid_8,uuid_9,uuid_10,uuid_11)



#define X_POTATO_REGISTER_CLASS(classname)\
	XSingletonFactory<classname> classname::s_factory;


#endif // !_X_POTATO_EXTENSION_MACROS_H_
