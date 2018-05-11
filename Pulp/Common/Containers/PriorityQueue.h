#pragma once

X_NAMESPACE_BEGIN(core)

template<typename T, class ContainerT = ArrayGrowMultiply<T>, class Pr = std::less<typename ContainerT::value_type>>
class PriorityQueue
{
public:
    typedef PriorityQueue<T, ContainerT, Pr> MyType;
    typedef ContainerT container_type;
    typedef typename ContainerT::Type Type;
    typedef typename ContainerT::value_type value_type;
    typedef typename ContainerT::size_type size_type;
    typedef typename ContainerT::reference reference;
    typedef typename ContainerT::const_reference const_reference;

public:
    PriorityQueue(core::MemoryArenaBase* arena);
    PriorityQueue(const MyType& oth);
    PriorityQueue(MyType&& oth);

    MyType& operator=(const MyType& rhs);
    MyType& operator=(MyType&& rhs);

    void clear(void);
    void free(void);

    void push(const value_type& _Val);
    void push(value_type&& _Val);
    template<class... _Valty>
    void emplace(_Valty&&... _Val);

    void pop(void);

    const_reference peek(void) const;

    bool isEmpty(void) const;
    bool isNotEmpty(void) const;

    size_type size(void) const;
    size_type capacity(void) const;

    void swap(MyType& oth);

private:
    ContainerT container_;
    Pr comp_;
};

X_NAMESPACE_END

#include "PriorityQueue.inl"