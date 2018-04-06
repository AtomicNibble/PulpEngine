#pragma once

#ifndef X_CON_FIXEDARRAY_H
#define X_CON_FIXEDARRAY_H

X_NAMESPACE_BEGIN(core)

X_DISABLE_WARNING(4324) // structure was padded due to alignment specifier

template<typename T, size_t N>
class FixedArray
{
public:
    typedef T Type;
    typedef T value_type;
    typedef T* TypePtr;
    typedef const T* ConstTypePtr;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef size_t size_type;
    typedef T& Reference;
    typedef T& reference;
    typedef const T& ConstReference;
    typedef const T& const_reference;

    enum : size_type
    {
        invalid_index = static_cast<size_type>(-1)
    };

    FixedArray();
    FixedArray(const T& initalval);
    ~FixedArray(void);

    // index operators
    const T& operator[](size_type idx) const;
    T& operator[](size_type idx);

    // returns a pointer to the list
    T* ptr(void);
    const T* ptr(void) const;

    T* data(void);
    const T* data(void) const;

    // clear the list, no memory free
    inline void clear(void);

    template<class... Args>
    inline Type& AddOne(Args&&... args);

    // append element (same as push_back)
    inline size_type append(const T& obj);
    inline size_type append(T&& obj);
    // appends a item to the end, resizing if required.
    inline size_type push_back(const T& obj);
    inline size_type push_back(T&& obj);

    template<class... ArgsT>
    inline size_type emplace_back(ArgsT&&... args);

    // Removes the last element, it is deconstructed
    inline void pop_back(void);

    inline iterator insert(iterator position, const T& val);

    template<typename Iter>
    typename std::enable_if<
        std::is_pointer<Iter>::value && std::is_same<std::remove_cv_t<std::remove_pointer_t<Iter>>, Type>::value,
        iterator>::type
        insert(const_iterator pos, Iter first, Iter last);

    bool removeIndex(size_type idx);
    bool remove(iterator position);

    // returns invalid_index when not found.
    size_type find(const Type& val) const;

    // Inserts or erases elements at the end such that size is 'size'
    inline void resize(size_type size);
    inline void resize(size_type size, const T& t);

    // any iterms in the array
    inline bool isEmpty(void) const;
    inline bool isNotEmpty(void) const;
    inline bool empty(void) const; // for stl

    inline size_type size(void) const;
    // returns number of elements allocated for
    inline size_type capacity(void) const;

    // for STL use
    inline iterator begin(void);
    inline const_iterator begin(void) const;
    inline iterator end(void);
    inline const_iterator end(void) const;
    inline Reference front(void);
    inline ConstReference front(void) const;
    inline Reference back(void);
    inline ConstReference back(void) const;

private:
    size_type size_;
    uint8_t X_ALIGNED_SYMBOL(array_[N * sizeof(T)], X_ALIGN_OF(T));
};

X_ENABLE_WARNING(4324)

X_NAMESPACE_END

#include "FixedArray.inl"

#endif // X_CON_FIXEDARRAY_H
