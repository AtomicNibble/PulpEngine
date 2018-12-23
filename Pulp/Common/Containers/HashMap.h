#pragma once

#ifndef _X_CON_HASHMAP_H_
#define _X_CON_HASHMAP_H_

#include "HashBase.h"
#include "Util/HashHelper.h"

X_NAMESPACE_BEGIN(core)


template<class Key,
    class Value,
    class HashFn = hash<Key>,
    class EqualKey = equal_to<Key>>
class HashMap : public HashBase<Key, std::pair<const Key, Value>, HashFn, EqualKey>
{
public:
    typedef HashBase<Key, std::pair<const Key, Value>, HashFn, EqualKey> BaseType;

    typedef typename BaseType::key_type key_type;
    typedef Value data_type;
    typedef Value mapped_type;
    typedef typename BaseType::value_type value_type;
    typedef typename BaseType::hasher hasher;
    typedef typename BaseType::key_equal key_equal;

    typedef typename BaseType::size_type size_type;
    typedef typename BaseType::reference reference;
    typedef typename BaseType::const_reference const_reference;
    typedef typename BaseType::pointer pointer;
    typedef typename BaseType::const_pointer const_pointer;

    typedef typename BaseType::iterator iterator;
    typedef typename BaseType::const_iterator const_iterator;
    typedef typename BaseType::const_iterator const_iterator;
    typedef typename BaseType::Node Node;

    /// A constant defining the size of a single entry when stored in the hash map.
    static const size_t PER_ENTRY_SIZE = BaseType::PER_ENTRY_SIZE;

public:
    explicit HashMap(MemoryArenaBase* arena) :
        BaseType(arena)
    {
    }
    HashMap(MemoryArenaBase* arena, size_type num) :
        BaseType(arena, num)
    {
    }
    HashMap(const HashMap& oth) = default;
    HashMap(HashMap&& oth) = default;

    HashMap& operator=(const HashMap& oth) = default;
    HashMap& operator=(HashMap&& oth) = default;

    std::pair<iterator, bool> insert(const value_type& obj)
    {
        BaseType::ensureSize(BaseType::numElements_ + 1);
        return BaseType::insertUniqueNoResize(obj);
    }

    // reserver a number of elements
    void reserve(size_type num)
    {
        BaseType::ensureSize(num + 1);
    }

    data_type& operator[](const key_type& key)
    {
        return BaseType::find(key)->second;
    }

    bool contains(const key_type& key)
    {
        return BaseType::find(key) != BaseType::end();
    }

    template<class MemoryArenaT>
    static inline size_t GetMemoryRequirement(size_t capacity)
    {
        return (MemoryArenaT::getMemoryRequirement(sizeof(Node) * capacity) + MemoryArenaT::getMemoryRequirement(sizeof(Node*) * capacity));
    }

public:
};

X_NAMESPACE_END

#endif // !_X_CON_HASHMAP_H_
