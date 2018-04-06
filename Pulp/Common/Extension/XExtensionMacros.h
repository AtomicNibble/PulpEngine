#pragma once

#ifndef _X_POTATO_EXTENSION_MACROS_H_
#define _X_POTATO_EXTENSION_MACROS_H_

#define _STDINT // fuck standard types!
#include <memory>

#include "TypeList.h"

#include "IEngineUnknown.h"
#include "IEngineFactory.h"

#include "FactoryRegNode.h"

#include "GUID.h"

namespace Internal
{
    template<class Dst>
    struct InterfaceCast;

    template<class Dst>
    struct InterfaceCast
    {
        template<class T>
        static void* Op(T* p)
        {
            return (Dst*)p;
        }
    };

    template<>
    struct InterfaceCast<IEngineUnknown>
    {
        template<class T>
        static void* Op(T* p)
        {
            return const_cast<IEngineUnknown*>(static_cast<const IEngineUnknown*>(static_cast<const void*>(p)));
        }
    };
} // namespace Internal

template<class TList>
struct InterfaceCast;

template<>
struct InterfaceCast<TL::NullType>
{
    template<class T>
    static void* Op(T*, const EngineGUID&)
    {
        return nullptr;
    }
};

template<class Head, class Tail>
struct InterfaceCast<TL::Typelist<Head, Tail>>
{
    template<class T>
    static void* Op(T* p, const EngineGUID& iid)
    {
        if (EngineIdOf<Head>() == iid) {
            return Internal::InterfaceCast<Head>::Op(p);
        }
        return InterfaceCast<Tail>::Op(p, iid);
    }
};

template<class TList>
struct FillIIDs;

template<>
struct FillIIDs<TL::NullType>
{
    static void Op(EngineGUID*)
    {
    }
};

template<class Head, class Tail>
struct FillIIDs<TL::Typelist<Head, Tail>>
{
    static void Op(EngineGUID* p)
    {
        *p++ = EngineIdOf<Head>();
        FillIIDs<Tail>::Op(p);
    }
};

template<typename T>
class XFactory : public IEngineFactory
{
public:
    XFactory() :
        numIIDs_(0),
        pIIDs_(nullptr),
        registerFactory_()
    {
        static EngineGUID supportedIIDs[TL::Length<typename T::FullInterfaceList>::value];
        FillIIDs<typename T::FullInterfaceList>::Op(supportedIIDs);
        pIIDs_ = &supportedIIDs[0];
        numIIDs_ = TL::Length<typename T::FullInterfaceList>::value;

        new (&registerFactory_) XRegFactoryNode(this);
    }

    virtual const char* GetName(void) const X_OVERRIDE
    {
        return T::GetSName();
    }

    virtual const EngineGUID& GetGUID(void) const X_OVERRIDE
    {
        return T::GetSGUID();
    }

    virtual bool ClassSupports(const EngineGUID& iid) const X_OVERRIDE
    {
        for (size_t i = 0; i < numIIDs_; ++i) {
            if (iid == pIIDs_[i]) {
                return true;
            }
        }
        return false;
    }

public:
    virtual IEngineUnknown* CreateInstance(void) const X_OVERRIDE
    {
        return new T;
    }

private:
    XFactory(const XFactory&);
    XFactory& operator=(const XFactory&);

private:
    size_t numIIDs_;
    EngineGUID* pIIDs_;
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

    virtual IEngineUnknown* CreateInstance(void) const X_OVERRIDE
    {
        static IEngineUnknown* pInst = XFactory<T>::CreateInstance();
        return pInst;
    }
};

// #define ENGINE_FACTORY_DECLARE(implclassname) \
//   private:                                 \
//     static XFactory<implclassname> s_factory;

#define _ENGINE_FACTORY_DECLARE_SINGLETON(implclassname) \
private:                                                 \
    friend class XFactory<implclassname>;                \
    static XSingletonFactory<implclassname> s_factory;

#define _ENGINE_IMPLEMENT_IENGINEUNKNOWN()                             \
public:                                                                \
    virtual IEngineFactory* GetFactory() const X_OVERRIDE              \
    {                                                                  \
        return &s_factory;                                             \
    }                                                                  \
                                                                       \
protected:                                                             \
    virtual void* QueryInterface(const EngineGUID& iid) const override \
    {                                                                  \
        return InterfaceCast<FullInterfaceList>::Op(this, iid);        \
    }

#define _ENGINE_ADD_MEMBERS(classname, cname, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5, uuid_6, uuid_7, uuid_8, uuid_9, uuid_10, uuid_11) \
public:                                                                                                                                 \
    static const char* GetSName(void)                                                                                                   \
    {                                                                                                                                   \
        return cname;                                                                                                                   \
    }                                                                                                                                   \
    static const EngineGUID& GetSGUID(void)                                                                                             \
    {                                                                                                                                   \
        static const EngineGUID id(uuid_1, uuid_2, uuid_3, uuid_4, uuid_5, uuid_6, uuid_7, uuid_8, uuid_9, uuid_10, uuid_11);           \
        return id;                                                                                                                      \
    }                                                                                                                                   \
    static std::shared_ptr<classname> CreateInstance(void)                                                                              \
    {                                                                                                                                   \
        IEngineUnknown* p = s_factory.CreateInstance();                                                                                 \
        return std::shared_ptr<classname>(*static_cast<std::shared_ptr<classname>*>(static_cast<void*>(&p)));                           \
    }                                                                                                                                   \
                                                                                                                                        \
protected:                                                                                                                              \
    classname();                                                                                                                        \
    virtual ~classname();

#define X_ENGINE_INTERFACE_BEGIN() \
private:                           \
    typedef TL::MakeTypelist < IEngineUnknown

#define X_ENGINE_INTERFACE_ADD(iname) , iname

#define X_ENGINE_INTERFACE_END()                 \
    > ::Result _UserDefinedPartialInterfaceList; \
                                                 \
protected:                                       \
    typedef TL::NoDuplicates<_UserDefinedPartialInterfaceList>::Result FullInterfaceList;

#define X_ENGINE_INTERFACE_SIMPLE(iname) \
    X_ENGINE_INTERFACE_BEGIN()           \
    X_ENGINE_INTERFACE_ADD(iname)        \
    X_ENGINE_INTERFACE_END()

#define X_ENGINE_GENERATE_SINGLETONCLASS(classname, cname, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5, uuid_6, uuid_7, uuid_8, uuid_9, uuid_10, uuid_11) \
    _ENGINE_FACTORY_DECLARE_SINGLETON(classname)                                                                                                     \
    _ENGINE_IMPLEMENT_IENGINEUNKNOWN()                                                                                                               \
    _ENGINE_ADD_MEMBERS(classname, cname, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5, uuid_6, uuid_7, uuid_8, uuid_9, uuid_10, uuid_11)

#define X_ENGINE_REGISTER_CLASS(classname) \
    XSingletonFactory<classname> classname::s_factory;

#endif // !_X_POTATO_EXTENSION_MACROS_H_
