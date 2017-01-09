#pragma once


#ifndef _X_POTATO_CLASS_I_H_
#define _X_POTATO_CLASS_I_H_


#include "GUID.h"

struct IPotatoFactory;



#define _POTATO_INTERFACE_ADD_GUID(name, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5,uuid_6,uuid_7,uuid_8,uuid_9,uuid_10,uuid_11) \
static const PotatoGUID& IGUID(void)  \
{ \
static const PotatoGUID id(uuid_1, uuid_2, uuid_3, uuid_4, uuid_5, uuid_6, uuid_7, uuid_8, uuid_9, uuid_10, uuid_11);		\
return id;  \
} 



template<class T>
X_INLINE const PotatoGUID& PotatoIdOf(void)
{
	return T::IGUID();
}

namespace InterfaceSemantic
{

	template<class Dst, class Src>
	X_INLINE Dst* interface_cast(Src* p)
	{
		return static_cast<Dst*>(p ? p->QueryInterface(PotatoIdOf<Dst>()) : 0);
	}

	template<class Dst, class Src>
	X_INLINE Dst* interface_cast(const Src* p)
	{
		return static_cast<const Dst*>(p ? p->QueryInterface(PotatoIdOf<Dst>()) : 0);
	}


} // InterfaceCast

#define _BEFRIEND_POTATOINTERFACE_CAST()                                                 \
  template<class Dst, class Src> friend Dst * InterfaceSemantic::interface_cast(Src*);       \
  template<class Dst, class Src> friend Dst * InterfaceSemantic::interface_cast(const Src*); \


#define _BEFRIEND_DELETER(iname) \
  friend struct std::default_delete<iname>;

#define _PROTECTED_DTOR(iname) \
  public:                   \
    virtual ~iname() {}


#define POTATO_INTERFACE_DECLARE(name, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5,uuid_6,uuid_7,uuid_8,uuid_9,uuid_10,uuid_11) \
	_POTATO_INTERFACE_ADD_GUID(name, uuid_1, uuid_2, uuid_3, uuid_4, uuid_5,uuid_6,uuid_7,uuid_8,uuid_9,uuid_10,uuid_11) \
	_BEFRIEND_POTATOINTERFACE_CAST() \
	_PROTECTED_DTOR(name) \
	public:


struct IPotatoUnknown
{
	POTATO_INTERFACE_DECLARE(IPotatoUnknown, 0x59d0dd8, 0x9969, 0x4a89, 0xbf, 0xea, 0xdb, 0xa8, 0x8f, 0x4, 0xd, 0x46 );

	virtual IPotatoFactory* GetFactory(void) const X_ABSTRACT;

protected:
	virtual void* QueryInterface(const PotatoGUID& iid) const X_ABSTRACT;
};


#endif // !_X_POTATO_CLASS_I_H_
