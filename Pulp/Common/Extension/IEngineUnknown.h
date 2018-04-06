#pragma once

#include "GUID.h"

struct IEngineFactory;

#define _ENGINE_INTERFACE_ADD_GUID(name, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5, uuid_6, uuid_7, uuid_8, uuid_9, uuid_10, uuid_11) \
    static const EngineGUID& IGUID(void)                                                                                           \
    {                                                                                                                              \
        static const EngineGUID id(uuid_1, uuid_2, uuid_3, uuid_4, uuid_5, uuid_6, uuid_7, uuid_8, uuid_9, uuid_10, uuid_11);      \
        return id;                                                                                                                 \
    }

template<class T>
X_INLINE const EngineGUID& EngineIdOf(void)
{
    return T::IGUID();
}

namespace InterfaceSemantic
{
    template<class Dst, class Src>
    X_INLINE Dst* interface_cast(Src* p)
    {
        return static_cast<Dst*>(p ? p->QueryInterface(EngineIdOf<Dst>()) : 0);
    }

    template<class Dst, class Src>
    X_INLINE Dst* interface_cast(const Src* p)
    {
        return static_cast<const Dst*>(p ? p->QueryInterface(EngineIdOf<Dst>()) : 0);
    }

} // namespace InterfaceSemantic

#define _BEFRIEND_POTATOINTERFACE_CAST()                 \
    template<class Dst, class Src>                       \
    friend Dst* InterfaceSemantic::interface_cast(Src*); \
    template<class Dst, class Src>                       \
    friend Dst* InterfaceSemantic::interface_cast(const Src*);

#define _BEFRIEND_DELETER(iname) \
    friend struct std::default_delete<iname>;

#define _PROTECTED_DTOR(iname) \
public:                        \
    virtual ~iname()           \
    {                          \
    }

#define ENGINE_INTERFACE_DECLARE(name, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5, uuid_6, uuid_7, uuid_8, uuid_9, uuid_10, uuid_11) \
    _ENGINE_INTERFACE_ADD_GUID(name, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5, uuid_6, uuid_7, uuid_8, uuid_9, uuid_10, uuid_11)   \
    _BEFRIEND_POTATOINTERFACE_CAST()                                                                                             \
    _PROTECTED_DTOR(name)                                                                                                        \
public:

struct IEngineUnknown
{
    ENGINE_INTERFACE_DECLARE(IEngineUnknown, 0x59d0dd8, 0x9969, 0x4a89, 0xbf, 0xea, 0xdb, 0xa8, 0x8f, 0x4, 0xd, 0x46);

    virtual IEngineFactory* GetFactory(void) const X_ABSTRACT;

protected:
    virtual void* QueryInterface(const EngineGUID& iid) const X_ABSTRACT;
};
