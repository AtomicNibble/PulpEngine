#pragma once

#ifndef X_UTIL_SCOPED_POINTER_H_
#define X_UTIL_SCOPED_POINTER_H_

#include <CompileTime\IsPointer.h>

X_NAMESPACE_BEGIN(core)

template<typename T>
class UniquePointerBase
{
public:
    typedef T* pointer;

    X_INLINE UniquePointerBase(core::MemoryArenaBase* arena, T* pInstance);
    X_INLINE UniquePointerBase(UniquePointerBase&& oth);

    X_INLINE core::MemoryArenaBase* getArena(void) const;
    X_INLINE pointer& ptr(void);
    X_INLINE const pointer& ptr(void) const;

protected:
    X_INLINE void swap(UniquePointerBase& oth);

private:
    pointer pInstance_;

protected:
    core::MemoryArenaBase* arena_;
};

template<typename T>
class UniquePointer : public UniquePointerBase<T>
{
public:
    typedef UniquePointer<T> MyT;
    typedef UniquePointerBase<T> Mybase;
    typedef typename Mybase::pointer pointer;

    template<typename T2>
    using is_convertible = std::is_convertible<typename UniquePointer<T2>::pointer, pointer>;

    template<typename T2>
    using can_assign = std::enable_if<!std::is_array<T2>::value && is_convertible<T2>::value, MyT&>;

    X_INLINE UniquePointer();
    X_INLINE explicit UniquePointer(core::MemoryArenaBase* arena);
    X_INLINE explicit UniquePointer(core::MemoryArenaBase* arena, nullptr_t);

    X_INLINE UniquePointer(core::MemoryArenaBase* arena, pointer pInstance);
    X_INLINE UniquePointer(UniquePointer&& oth);

    template<class T2, class = typename can_assign<T2>::type>
    X_INLINE explicit UniquePointer(UniquePointer<T2>&& oth);

    X_INLINE ~UniquePointer();

    X_INLINE UniquePointer& operator=(nullptr_t);
    X_INLINE UniquePointer& operator=(UniquePointer&& rhs);

    template<class T2>
    X_INLINE typename can_assign<T2>::type
        operator=(UniquePointer<T2>&& rhs);

    X_INLINE T& operator*() const;
    X_INLINE pointer operator->() const;
    X_INLINE pointer get(void) const;
    X_INLINE explicit operator bool() const;

    X_INLINE pointer release(void);
    X_INLINE void reset(pointer ptr = pointer());

    X_INLINE void swap(UniquePointer& oth);

private:
    void deleter(pointer ptr)
    {
        X_DELETE(const_cast<typename std::remove_const<T>::type*>(ptr), Mybase::getArena());
    }
};

template<typename T>
class UniquePointer<T[]> : public UniquePointerBase<T>
{
public:
    typedef UniquePointer<T[]> MyT;
    typedef UniquePointerBase<T> Mybase;
    typedef typename Mybase::pointer pointer;

    X_INLINE UniquePointer();
    X_INLINE explicit UniquePointer(core::MemoryArenaBase* arena);
    X_INLINE explicit UniquePointer(core::MemoryArenaBase* arena, nullptr_t);

    X_INLINE UniquePointer(core::MemoryArenaBase* arena, pointer pInstance);

    X_INLINE UniquePointer(UniquePointer&& oth);
    X_INLINE ~UniquePointer();

    X_INLINE UniquePointer& operator=(nullptr_t);
    X_INLINE UniquePointer& operator=(UniquePointer&& rhs);

    X_INLINE T& operator*() const;
    X_INLINE pointer operator->() const;
    X_INLINE pointer get(void) const;
    X_INLINE explicit operator bool() const;

    X_INLINE pointer release(void);
    X_INLINE void reset(pointer ptr = pointer());

    X_INLINE void swap(UniquePointer& oth);

private:
    void deleter(pointer ptr)
    {
        X_DELETE_ARRAY(const_cast<typename std::remove_const<T>::type*>(ptr), Mybase::getArena());
    }
};

template<typename T, class... _Types>
X_INLINE typename std::enable_if<!std::is_array<T>::value, core::UniquePointer<T>>::type
    makeUnique(core::MemoryArenaBase* arena, _Types&&... _Args)
{
    return core::UniquePointer<T>(arena, X_NEW(T, arena, "makeUnique<T>")(std::forward<_Types>(_Args)...));
}

template<typename T, class... _Types>
X_INLINE typename std::enable_if<!std::is_array<T>::value, core::UniquePointer<T>>::type
    makeUnique(core::MemoryArenaBase* arena, const char* pID, _Types&&... _Args)
{
    return core::UniquePointer<T>(arena, X_NEW(T, arena, pID)(std::forward<_Types>(_Args)...));
}

template<typename T, class... _Types>
X_INLINE typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0, core::UniquePointer<T>>::type
    makeUnique(core::MemoryArenaBase* arena, size_t size)
{
    typedef typename std::remove_extent<T>::type _ElemT;
    return core::UniquePointer<T>(arena, X_NEW_ARRAY(_ElemT, size, arena, "makeUnique<T[]>"));
}

template<typename T, class... _Types>
X_INLINE typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0, core::UniquePointer<T>>::type
    makeUnique(core::MemoryArenaBase* arena, size_t size, size_t alignment)
{
    typedef typename std::remove_extent<T>::type _ElemT;
    return core::UniquePointer<T>(arena, X_NEW_ARRAY_ALIGNED(_ElemT, size, arena, "makeUnique<T[]>", alignment));
}

X_NAMESPACE_END

#include "UniquePointer.inl"

#endif // !X_UTIL_SCOPED_POINTER_H_