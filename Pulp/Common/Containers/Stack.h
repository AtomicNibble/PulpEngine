#pragma once

#ifndef _X_CON_STACK_H_
#define _X_CON_STACK_H_

X_NAMESPACE_BEGIN(core)

//
// Provides a stack style container.
//
// it's FILO.
//
//
template<typename T>
class Stack
{
public:
    typedef T Type;
    typedef T* TypePtr;
    typedef const T* ConstTypePtr;
    typedef T* Iterator;
    typedef const T* ConstIterator;
    typedef size_t size_type;

    // constructs the Stack no memory is allocated.
    inline Stack(MemoryArenaBase* arena);
    // constast stack with space for numElements
    inline Stack(MemoryArenaBase* arena, size_type numElements);

    inline ~Stack();

    inline void SetArena(MemoryArenaBase* arena);

    // push a value onto the stack
    inline void push(const T& val);
    inline void push(T&& obj);

    // emplace a value onto the stack
    template<class... ArgsT>
    inline void emplace(ArgsT&&... args);
    // pop a value of the stack
    inline void pop(void);
    // get the top item without removing
    inline T& top(void);
    // get the top item without removing
    inline const T& top(void) const;

    // resizes the object
    inline void reserve(size_type size);
    // free's the memory associated with the stack.
    inline void free(void);
    // clears all objects but dose not free memory.
    inline void clear(void);

    // returns the number of elemets in the stack currently
    inline size_type size(void) const;
    // returns the number of elements this stack can currently hold.
    inline size_type capacity(void) const;

    inline bool isEmpty(void) const;
    inline bool isNotEmpty(void) const;

    // STL Iterators.
    inline Iterator begin(void);
    inline ConstIterator begin(void) const;
    inline Iterator end(void);
    inline ConstIterator end(void) const;

private:
    X_NO_COPY(Stack);
    X_NO_ASSIGN(Stack);

    inline void Delete(T* pData);
    inline T* Allocate(size_type num);

    T* current_;
    T* start_;
    T* end_;

    MemoryArenaBase* arena_;
};

#include "Stack.inl"

X_NAMESPACE_END

#endif // !_X_CON_STACK_H_
