#pragma once

#include <Util/BitUtil.h>
#include <Memory/NewAndDelete.h>

#include "HashMap.h" // TODO: move hash out
#include <iterator>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>

X_NAMESPACE_BEGIN(core)

// Simple hash table that can't resize and uses linera probing.

template <typename HashTableT, typename IterVal> 
struct HashTableIterator 
{
    friend HashTableT;

    using difference_type = std::ptrdiff_t;
    using value_type = IterVal;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;
    using size_type = typename HashTableT::size_type;


private:
    explicit HashTableIterator(HashTableT* pHashTable) :
        pHashTable_(pHashTable),
        idx_(0)
    { 
        skipPastEmpty();
    }
    
    explicit HashTableIterator(HashTableT* pHashTable, size_type idx) :
        pHashTable_(pHashTable),
        idx_(idx) 
    {
    }

public:

    bool operator==(const HashTableIterator& other) const {
        return other.pHashTable_ == pHashTable_ && other.idx_ == idx_;
    }
    bool operator!=(const HashTableIterator& other) const {
        return !(other == *this);
    }

    HashTableIterator &operator++() {
        ++idx_;
        skipPastEmpty();
        return *this;
    }

    reference operator*() const { return pHashTable_->pData_[idx_]; }
    pointer operator->() const { return &pHashTable_->pData_[idx_]; }

private:

    void skipPastEmpty()
    {
        while (idx_ < pHashTable_->num_ && pHashTable_->isIndexEmpty(idx_)) {
            ++idx_;
        }
    }

private:
    HashTableT* pHashTable_;
    size_type idx_;
};


template<typename Key, typename Value, class Hash, class KeyEqual>
class FixedHashTableBase
{
protected:

public:
    using size_type = size_t;
    using key_type = Key;
    using mapped_type = Value;    
    using value_type = std::pair<Key, Value>;
    using hasher = Hash;
    using key_equal = KeyEqual;

    using iterator = HashTableIterator<FixedHashTableBase, value_type>;
    using const_iterator = HashTableIterator<const FixedHashTableBase, const value_type>;
    
    friend struct HashTableIterator<FixedHashTableBase, value_type>;
    friend struct HashTableIterator<const FixedHashTableBase, const value_type>;

    typedef std::pair<iterator, bool> return_pair;

public:
    FixedHashTableBase(value_type* pData, size_type maxItems);
    ~FixedHashTableBase();

    void clear(void);

    template <class... Args>
    return_pair emplace(Args&&... args);

    return_pair insert(const value_type& val);
    return_pair insert(value_type&& val);

    iterator find(const key_type& k);
    const_iterator find(const key_type& k) const;

    template <typename K> 
    iterator find(const K& x);
    template <typename K> 
    const_iterator find(const K& x) const;

    void erase(iterator it);
    size_type erase(const key_type& key);
    template <typename K> 
    size_type erase(const K& x);

    mapped_type& operator[] (const key_type& key);
    mapped_type& operator[] (key_type&& key);

    size_type size(void) const;
    size_type capacity(void) const;

 //   float loadFactor(void) const;

    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;

    const_iterator cbegin() const;
    const_iterator cend() const;

private:

    template <typename K, typename... Args>
    return_pair emplace_impl(const K& key, Args&&... args) 
    {
        for (size_type idx = key2idx(key); ; idx = probeNext(idx))
        {
            if(isIndexEmpty(idx))
            {
                Mem::Construct<value_type>(&pData_[idx], key, std::forward<Args>(args)...);
                size_++;
                return { iterator(this, idx), true };
            }
            else if (key_equal()(pData_[idx].first, key)) {
                return { iterator(this, idx), false };
            }
        }
    }

    template <typename K> 
    iterator find_impl(const K& key) 
    {
        for (size_type idx = key2idx(key); ; idx = probeNext(idx))
        {
            if (isIndexEmpty(idx)) {
                return end();
            }
            if (key_equal()(pData_[idx].first, key)) {
                return iterator(this, idx);
            }
        }
    }


    void erase_impl(iterator it) 
    {
        size_type bucket = it.idx_;
        for (size_type idx = probeNext(bucket);; idx = probeNext(idx))
        {
            if (isIndexEmpty(idx)) {
                destroyIndex(bucket);
                size_--;
                return;
            }

            size_type ideal = key2idx(pData_[idx].first);

            if (diff(bucket, ideal) < diff(idx, ideal)) {
                // swap, bucket is closer to ideal than idx
                pData_[bucket] = pData_[idx];
                bucket = idx;
            }
        }
    }

    template <typename K>
    size_type key2idx(const K& key) const
    {
        return hasher()(key) & mask_;
    }

    size_type probeNext(size_type idx) const {
        return (idx + 1) & mask_;
    }

    size_type diff(size_type a, size_type b) const {
        return (num_ + (a - b)) & mask_;
    }

    bool isIndexEmpty(size_type idx) const {
        return std::memcmp(&pData_[idx], &emptyEntry_, sizeof(emptyEntry_)) == 0;
    }

    void destroyIndex(size_type idx) {
        Mem::Destruct<value_type>(&pData_[idx]);
        std::memset(&pData_[idx], -1, sizeof(value_type));

    }


protected:
    uint8_t X_ALIGNED_SYMBOL(emptyEntry_[sizeof(value_type)], X_ALIGN_OF(value_type));

    value_type* pData_; // uninitialized
    Hash hash_;
    const size_type num_;
    size_type size_;
    const size_type mask_;
};


template<typename Key, typename Value, class Hash, class KeyEqual>
class FixedHashTableOwningPolicy : public FixedHashTableBase<Key, Value, Hash, KeyEqual>
{
    using BaseT = FixedHashTableBase<Key, Value, Hash, KeyEqual>;

public:
    FixedHashTableOwningPolicy(core::MemoryArenaBase* arena, size_type maxItem);
    ~FixedHashTableOwningPolicy();

private:
    core::MemoryArenaBase* arena_;
};

template<size_t N, typename Key, typename Value, class Hash, class KeyEqual>
class FixedHashTableStackPolicy : public FixedHashTableBase<Key, Value, Hash, KeyEqual>
{
    using BaseT = FixedHashTableBase<Key, Value, Hash, KeyEqual>;

public:
    FixedHashTableStackPolicy();

private:
    uint8_t X_ALIGNED_SYMBOL(array_[N * sizeof(BaseT::value_type)], X_ALIGN_OF(BaseT::value_type));
};


template<class StorageType>
class FixedHashTablePolicyBase : public StorageType
{
public:
    typedef typename StorageType::size_type size_type;


public:
    using StorageType::StorageType;
};

template<typename Key, typename Value, class Hash = hash<Key>, class KeyEqual = equal_to<Key>>
using FixedHashTable = FixedHashTablePolicyBase<FixedHashTableOwningPolicy<Key, Value, Hash, KeyEqual>>;

template<size_t N, typename Key, typename Value, class Hash = hash<Key>, class KeyEqual = equal_to<Key>>
using FixedHashTableStack = FixedHashTablePolicyBase<FixedHashTableStackPolicy<N, Key, Value, Hash, KeyEqual>>;


X_NAMESPACE_END

#include "FixedHashTable.inl"