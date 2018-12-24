#pragma once

#include <Util/BitUtil.h>
#include <Util/HashHelper.h>
#include <Memory/NewAndDelete.h>

#include <iterator>

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
    FixedHashTableBase(const FixedHashTableBase& oth) = delete;
    FixedHashTableBase(FixedHashTableBase&& oth);
    FixedHashTableBase(uint8_t* pData, size_type maxItems);
    ~FixedHashTableBase();

    FixedHashTableBase& operator=(FixedHashTableBase&& oth) = delete; // only allow move construct currently
    FixedHashTableBase& operator=(const FixedHashTableBase& oth) = delete;

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

    bool empty(void) const;
    bool isEmpty(void) const;
    bool isNotEmpty(void) const;

    hasher hash_function(void) const;
    key_equal key_eq(void) const;

    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;

    const_iterator cbegin() const;
    const_iterator cend() const;

private:
    template <typename K, typename... Args>
    return_pair emplace_impl(const K& key, Args&&... args);
    template <typename K, typename... Args>
    return_pair emplace_impl(K&& key, Args&&... args);

    template <typename K> 
    iterator find_impl(const K& key);
    template <typename K>
    const_iterator find_impl(const K& key) const;

    void erase_impl(iterator it);

    template <typename K>
    size_type erase_impl(const K &key);

    template <typename K>
    size_type key2idx(const K& key) const;
    size_type probeNext(size_type idx) const;
    size_type diff(size_type a, size_type b) const;

    bool isIndexEmpty(size_type idx) const;
    void destroyIndex(size_type idx);

protected:
    uint8_t X_ALIGNED_SYMBOL(emptyEntry_[sizeof(value_type)], X_ALIGN_OF(value_type));

    value_type* pData_; // uninitialized
    size_type size_;
    const size_type num_;
    const size_type mask_;
};


template<typename Key, typename Value, class Hash, class KeyEqual>
class FixedHashTableOwningPolicy : public FixedHashTableBase<Key, Value, Hash, KeyEqual>
{
    using BaseT = FixedHashTableBase<Key, Value, Hash, KeyEqual>;

public:
    FixedHashTableOwningPolicy(core::MemoryArenaBase* arena, size_type maxItem);
    ~FixedHashTableOwningPolicy();

    void free(void);

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