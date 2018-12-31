#pragma once

#ifndef X_CON_ARRAY_H
#define X_CON_ARRAY_H

#include <ISerialize.h>

#include "CompileTime\IsPOD.h"
#include "Containers\ArrayAllocators.h"

X_NAMESPACE_BEGIN(core)

class MemoryArenaBase;
struct XFile;

template<typename T, class Allocator = ArrayAllocator<T>, class GrowPolicy = growStrat::Linear>
class Array : public GrowPolicy
{
public:
    typedef T Type;
    typedef T value_type;
    typedef T* TypePtr;
    typedef T* pointer;
    typedef const T* ConstTypePtr;
    typedef const T* const_pointer;
    typedef T* Iterator;
    typedef const T* ConstIterator;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef size_t size_type;
    typedef T& Reference;
    typedef T& reference;
    typedef const T& ConstReference;
    typedef const T& const_reference;
    typedef Array<T, Allocator, GrowPolicy> MyT;

    enum : size_type
    {
        invalid_index = static_cast<size_type>(-1)
    };

    Array(MemoryArenaBase* arena);
    Array(MemoryArenaBase* arena, size_type size);
    Array(MemoryArenaBase* arena, size_type size, const T& initialValue);
    Array(MemoryArenaBase* arena, std::initializer_list<T> iList);
    Array(const Array& oth);
    Array(Array&& oth);
    ~Array(void);

    core::MemoryArenaBase* getArena(void) const; // have one use case for this currently lol.

    Allocator& getAllocator(void);
    const Allocator& getAllocator(void) const;

    MyT& operator=(std::initializer_list<T> iList);
    MyT& operator=(const MyT& oth);
    MyT& operator=(MyT&& oth);

    // index operators
    const T& operator[](size_type idx) const;
    T& operator[](size_type idx);

    // returns a pointer to the list
    T* ptr(void);
    const T* ptr(void) const;
    T* data(void);
    const T* data(void) const;

    X_INLINE const bool isEmpty(void) const;
    X_INLINE const bool isNotEmpty(void) const;

    // clear the list, no memory free
    void clear(void);
    // clear the list, and Free memory
    void free(void);
    // attempts to make capacity == size, not guaranteed.
    void shrinkToFit(void);
    // returns number of elements in list
    size_type size(void) const;
    // returns number of elements allocated for
    size_type capacity(void) const;

    // Inserts or erases elements at the end such that size is 'size'
    void resize(size_type size);
    void resize(size_type size, const T& t);
    // increases the capacity to the given amount, only if it's greater than current.
    // dose not increase the amount of items.
    void reserve(size_type size);

    template<class... Args>
    Type& AddOne(Args&&... args);

    // append element (same as push_back)
    size_type append(const T& obj);
    size_type append(T&& obj);
    // add the list
    size_type append(const MyT& oth);
    size_type append(MyT&& oth);
    // appends a item to the end, resizing if required.
    size_type push_back(const T& obj);
    size_type push_back(T&& obj);

    template<class... ArgsT>
    size_type emplace_back(ArgsT&&... obj);

    // Removes the last element, it is deconstructed
    void pop_back(void);

    // insert the element at the given index
    size_type insertAtIndex(size_type idx, const Type& obj);
    size_type insertAtIndex(size_type idx, Type&& obj);

    Iterator insert(ConstIterator pos, const Type& obj);
    Iterator insert(ConstIterator pos, Type&& obj);
    Iterator insert(ConstIterator pos, size_type count, const Type& obj);

    template<typename Iter>
    typename std::enable_if<
        std::is_pointer<Iter>::value && std::is_same<std::remove_cv_t<std::remove_pointer_t<Iter>>, Type>::value,
        Iterator>::type
        insert(ConstIterator pos, Iter first, Iter last);

    template<class... Args>
    Iterator emplace(ConstIterator pos, Args&&... args);

    // inserts keep array sorted.
    Iterator insertSorted(const Type& obj);
    template<class Compare>
    Iterator insertSorted(const Type& obj, Compare comp);

    // remove the element at the given index, none-stable
    bool removeIndex(size_type idx);
    void remove(ConstIterator it);
    void remove(const T& item); // finds the item then calls removeIndex.

    // stable remove
    void removeIndexStable(size_type idx);
    void removeStable(const T& item);

    Iterator erase(ConstIterator first);
    Iterator erase(ConstIterator first, ConstIterator last);

    // returns invalid_index when not found.
    size_type find(const Type& val) const;

    ConstIterator findSorted(const Type& val) const;
    template<class Compare>
    ConstIterator findSorted(const Type& val, Compare comp) const;
    template<typename KeyType, class Compare, class CompareGt>
    ConstIterator findSortedKey(const KeyType& val, Compare comp, CompareGt compGreater) const;

    // swaps the arrays
    // A = B & B = A
    void swap(Array& oth);

    // for STL use
    inline Iterator begin(void);
    inline ConstIterator begin(void) const;
    inline Iterator end(void);
    inline ConstIterator end(void) const;
    inline Reference front(void);
    inline ConstReference front(void) const;
    inline Reference back(void);
    inline ConstReference back(void) const;

protected:
    // used to make sure buffer is large enougth.
    // if it grows exsisting values are copyied into new buffer.
    void ensureSize(size_type size);

    // for easy allocation custimisation.
    X_INLINE void DeleteArr(T* pArr);
    X_INLINE T* Allocate(size_type size);

protected:
    T* list_;        // pointer to memory block
    size_type num_;  // num elemets stored in the list
    size_type size_; // the current allocated size

    Allocator allocator_;
};

#include "Array.inl"

template<typename T>
using ArrayGrowMultiply = Array<T, ArrayAllocator<T>, growStrat::Multiply>;


X_NAMESPACE_END

#endif // X_CON_ARRAY_H
