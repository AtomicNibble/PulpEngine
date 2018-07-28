#pragma once
#ifndef X_CON_FIXEDSTACK_H_
#define X_CON_FIXEDSTACK_H_

X_NAMESPACE_BEGIN(core)

X_DISABLE_WARNING(4324) // structure was padded due to alignment specifier

template<typename T, size_t N>
class FixedStack
{
public:
    typedef T Type;
    typedef T* TypePtr;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef size_t size_type;

    inline FixedStack(void);
    inline ~FixedStack(void);

    inline void push(const T& value);
    inline void push(T&& value);

    template<class... ArgsT>
    inline void emplace(ArgsT&&... args);

    inline void pop(void);
    inline T& top(void);
    inline const T& top(void) const;

    inline bool isEmpty(void) const;

    inline void clear(void);

    inline size_type size(void) const;
    inline size_type capacity(void) const;

    inline iterator begin(void);
    inline const_iterator begin(void) const;
    inline iterator end(void);
    inline const_iterator end(void) const;

private:
    X_NO_COPY(FixedStack);
    X_NO_ASSIGN(FixedStack);

    uint8_t X_ALIGNED_SYMBOL(array_[N * sizeof(T)], X_ALIGN_OF(T));
    T* current_;
};

X_ENABLE_WARNING(4324)

#include "FixedStack.inl"

X_NAMESPACE_END

#endif // !X_CON_FIXEDSTACK_H_
