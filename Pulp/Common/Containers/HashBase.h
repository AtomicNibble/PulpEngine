#pragma once

#ifndef _CON_HASH_BASE_H_
#define _CON_HASH_BASE_H_

#include "Containers\Array.h"

X_NAMESPACE_BEGIN(core)

// Note: assumes long is at least 32 bits.
enum
{
    map_num_primes = 28
};

static const unsigned long __stl_prime_list[map_num_primes] = {
    53ul, 97ul, 193ul, 389ul, 769ul,
    1543ul, 3079ul, 6151ul, 12289ul, 24593ul,
    49157ul, 98317ul, 196613ul, 393241ul, 786433ul,
    1572869ul, 3145739ul, 6291469ul, 12582917ul, 25165843ul,
    50331653ul, 100663319ul, 201326611ul, 402653189ul, 805306457ul,
    1610612741ul, 3221225473ul, 4294967291ul};

template<typename T>
const T* lower_bound(const T* start, const T* end, T val)
{
    while (start < end) {
        if (*start >= val)
            return start;
        ++start;
    }
    return end;
}

inline unsigned long map_next_prime(unsigned long n)
{
    const unsigned long* __first = __stl_prime_list;
    const unsigned long* __last = __stl_prime_list + (int)map_num_primes;
    const unsigned long* pos = lower_bound(__first, __last, n);
    return pos == __last ? *(__last - 1) : *pos;
}

template<class Key, class Value, class HashFn, class EqualKey>
class HashBase;

template<class Key, class Value, class HashFn, class EqualKey>
struct _HashBase_iterator;

template<class Key, class Value, class HashFn, class EqualKey>
struct _HashBase_const_iterator;

template<class Value>
struct HashBase_node
{
    HashBase_node()
    {
    }
    HashBase_node(const Value& val) :
        val_(val)
    {
    }

    HashBase_node* next_;
    Value val_;
};

template<class Key, class Value, class HashFn, class EqualKey>
struct _HashBase_iterator
{
    typedef HashBase<Key, Value, HashFn, EqualKey> _HashMap;
    typedef _HashBase_iterator<Key, Value, HashFn, EqualKey> iterator;
    typedef _HashBase_const_iterator<Key, Value, HashFn, EqualKey> const_iterator;

    typedef HashBase_node<Value> Node;
    typedef Value value_type;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef Value& reference;
    typedef Value* pointer;

    _HashBase_iterator(Node* n, _HashMap* _map) :
        cur_(n),
        hm_(_map)
    {
    }
    _HashBase_iterator()
    {
    }

    reference operator*() const
    {
        return cur_->val_;
    }
    pointer operator->() const
    {
        return &(operator*());
    }
    iterator& operator++(void);
    iterator operator++(int);

    bool operator==(const iterator& rhs) const
    {
        return cur_ == rhs.cur_;
    }
    bool operator!=(const iterator& rhs) const
    {
        return cur_ != rhs.cur_;
    }

public:
    Node* cur_;
    _HashMap* hm_;
};

template<class Key, class Value, class HashFn, class EqualKey>
struct _HashBase_const_iterator
{
    typedef HashBase<Key, Value, HashFn, EqualKey> _HashMap;
    typedef _HashBase_iterator<Key, Value, HashFn, EqualKey> iterator;
    typedef _HashBase_const_iterator<Key, Value, HashFn, EqualKey> const_iterator;

    typedef HashBase_node<Value> Node;
    typedef Value value_type;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef const Value& reference;
    typedef const Value* pointer;

    _HashBase_const_iterator(const Node* n, const _HashMap* _map) :
        cur_(n),
        hm_(_map)
    {
    }
    _HashBase_const_iterator()
    {
    }
    _HashBase_const_iterator(const iterator& it) :
        cur_(it.cur_),
        hm_(it.hm_)
    {
    }

    reference operator*() const
    {
        return cur_->val_;
    }
    pointer operator->() const
    {
        return &(operator*());
    }
    const_iterator& operator++(void);
    const_iterator operator++(int);

    bool operator==(const const_iterator& rhs) const
    {
        return cur_ == rhs.cur_;
    }
    bool operator!=(const const_iterator& rhs) const
    {
        return cur_ != rhs.cur_;
    }

public:
    const Node* cur_;
    const _HashMap* hm_;
};

template<class Key,
    class Value,
    class HashFn,
    class EqualKey>
class HashBase
{
public:
    typedef Key key_type;
    typedef Value value_type;
    typedef HashFn hasher;
    typedef EqualKey key_equal;

    typedef size_t size_type;
    typedef value_type& reference;
    typedef const Value& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;

    typedef _HashBase_iterator<Key, value_type, HashFn, EqualKey> iterator;
    typedef _HashBase_const_iterator<Key, value_type, HashFn, EqualKey> const_iterator;

    friend struct _HashBase_iterator<Key, value_type, HashFn, EqualKey>;
    friend struct _HashBase_const_iterator<Key, value_type, HashFn, EqualKey>;

    typedef HashBase_node<value_type> Node;

public:
    static const size_t PER_ENTRY_SIZE = sizeof(Node) + sizeof(Node*);

    explicit HashBase(MemoryArenaBase* arena) :
        buckets_(arena),
        numElements_(0),
        arena_(arena)
    {
    }

    HashBase(MemoryArenaBase* arena, size_type num) :
        numElements_(0),
        buckets_(arena),
        arena_(arena)
    {
        X_ASSERT_NOT_NULL(arena);
        initialize_buckets(num);
    }

    HashBase(const HashBase& oth) = default;
    HashBase(HashBase&& oth) :
        equals_(oth.equals_),
        hasher_(oth.hasher_),
        numElements_(oth.numElements_),
        arena_(oth.arena_),
        buckets_(std::move(oth.buckets_))
    {
        oth.numElements_ = 0;
    }

    HashBase& operator=(const HashBase& oth) = default;
    HashBase& operator=(HashBase&& oth)
    {
        equals_ = oth.equals_;
        hasher_ = oth.hasher_;
        numElements_ = oth.numElements_;
        arena_ = oth.arena_;
        buckets_ = std::move(oth.buckets_);

        oth.numElements_ = 0;
        return *this;
    }

    ~HashBase()
    {
        clear();
    }

    void setArena(MemoryArenaBase* arena)
    {
        X_ASSERT(numElements_ == 0, "can't set hash map arena when it has items")(numElements_);
        arena_ = arena;
        buckets_.setArena(arena);
    }

    void setArena(MemoryArenaBase* arena, size_type size)
    {
        X_ASSERT(numElements_ == 0, "can't set hash map arena when it has items")(numElements_);
        arena_ = arena;
        buckets_.setArena(arena);

        initialize_buckets(size);
    }

    void clear(); // clear all the nodes, the buket list is not deleted.
    void free();  // clears all nodes and deletes the bucket.

    // find
    iterator find(const key_type& key)
    {
        size_type idx = bkt_num_key(key);
        Node* firstNode;
        for (firstNode = buckets_[idx];
             firstNode && !equals_(getKey(firstNode->val_), key);
             firstNode = firstNode->next_) {
        }
        return iterator(firstNode, this);
    }

    const_iterator find(const key_type& key) const
    {
        size_type idx = bkt_num_key(key);
        Node* firstNode;
        for (firstNode = buckets_[idx];
             firstNode && !equals_(getKey(firstNode->val_), key);
             firstNode = firstNode->next_) {
        }
        return const_iterator(firstNode, this);
    }

    // Erase
    void erase(iterator pos);
    size_type erase(const key_type& k);
    //	void erase(iterator first, iterator last);

public:
    size_type size() const
    {
        return numElements_;
    }
    size_type maxSize() const
    {
        return size_type(-1);
    }
    bool empty() const
    {
        return size() == 0;
    }

    void swap(HashBase& hash)
    {
        core::Swap(equals_, hash.equals_);
        core::Swap(hasher_, hash.hasher_);
        buckets_.swap(hash.buckets_);
        core::Swap(numElements_, hash.numElements_);
    }

    // Iterators.
    iterator begin()
    {
        for (size_type i = 0; i < buckets_.size(); ++i) {
            if (buckets_[i]) {
                return iterator(buckets_[i], this);
            }
        }
        return end();
    }

    iterator end()
    {
        return iterator(nullptr, this);
    }

    const_iterator begin() const
    {
        for (size_type i = 0; i < buckets_.size(); ++i) {
            if (buckets_[i]) {
                return const_iterator(buckets_[i], this);
            }
        }
        return end();
    }

    const_iterator end() const
    {
        return const_iterator(nullptr, this);
    }

protected:
    // insert
    std::pair<iterator, bool> insertUniqueNoResize(const value_type& obj);

    // Resize util
    void ensureSize(size_type size);

    size_type next_size(size_type num) const
    {
        return (size_type)map_next_prime((unsigned long)num);
    }

    // Bucket num
    size_type buketIndex(const value_type& obj) const
    {
        return bkt_num_key(getKey(obj));
    }

    size_type buketIndex(const key_type& key, size_t num) const
    {
        return hasher_(key) % num;
    }

    size_type buketIndex(const value_type& obj, size_t num) const
    {
        return bkt_num_key(getKey(obj), num);
    }

    // Key util
    const key_type& getKey(const value_type& obj) const
    {
        return obj.first;
    }

    size_type bkt_num_key(const key_type& key) const
    {
        return bkt_num_key(key, buckets_.size());
    }

    size_type bkt_num_key(const key_type& key, size_t num) const
    {
        return hasher_(key) % num;
    }

    // create / delete
    Node* newNode(const value_type& obj)
    {
        Node* node = X_NEW(Node, arena_, "HashBase<" X_PP_STRINGIZE(Key) "," X_PP_STRINGIZE(Value) ">")(obj);
        //	Node* node = (Node*)_aligned_malloc(sizeof(Node), 16);
        //		Mem::Construct(&node->val_, obj);
        node->next_ = 0;
        return node;
    }

    void deleteNode(Node* pNode)
    {
        X_DELETE(pNode, arena_);
        //	core::Mem::Destruct(pNode);
        //	_aligned_free(pNode);
    }

    void initialize_buckets(size_type __n)
    {
        const size_type __n_buckets = __n; // next_size(__n);
                                           //	buckets_.reserve(__n_buckets);
        buckets_.resize(__n_buckets);
        //	buckets_.insert(buckets_.end(), __n_buckets, (Node*)0);
        //	buckets_.insert(buckets_.end(), __n_buckets);
        numElements_ = 0;
    }

protected:
    key_equal equals_;
    HashFn hasher_;

    core::Array<Node*> buckets_;
    size_type numElements_;

    MemoryArenaBase* arena_;
};

template<class Key, class Value, class HashFn, class EqualKey>
void HashBase<Key, Value, HashFn, EqualKey>::clear()
{
    // delete all the nodes.
    for (size_type i = 0; i < buckets_.size(); i++) {
        Node* node = buckets_[i];
        while (node) {
            Node* next = node->next_;
            deleteNode(node);
            node = next;
        }
        buckets_[i] = nullptr;
    }
    numElements_ = 0;
}

template<class Key, class Value, class HashFn, class EqualKey>
void HashBase<Key, Value, HashFn, EqualKey>::free()
{
    clear();
    buckets_.free();
}

template<class Key, class Value, class HashFn, class EqualKey>
std::pair<typename HashBase<Key, Value, HashFn, EqualKey>::iterator, bool>
    HashBase<Key, Value, HashFn, EqualKey>::
        insertUniqueNoResize(const value_type& obj)
{
    const size_type idx = buketIndex(obj);
    Node* first = buckets_[idx];

    for (Node* cur = first; cur; cur = cur->next_) {
        if (equals_(getKey(cur->val_), getKey(obj))) {
            return std::pair<iterator, bool>(iterator(cur, this), false);
        }
    }

    Node* tmp = newNode(obj);
    tmp->next_ = first;
    buckets_[idx] = tmp;

    ++numElements_;

    return std::pair<iterator, bool>(iterator(tmp, this), true);
}

template<class Key, class Value, class HashFn, class EqualKey>
void HashBase<Key, Value, HashFn, EqualKey>::ensureSize(size_type _size)
{
    const size_type curent_size = buckets_.size();
    if (_size > curent_size) {
        // calculate the new size
        const size_type size = next_size(_size);

        core::Array<Node*> tmp(arena_, size, (Node*)(0));

        for (size_type bucket = 0; bucket < curent_size; ++bucket) {
            Node* firstNode = buckets_[bucket];
            while (firstNode) {
                size_type new_bucket = buketIndex(firstNode->val_, size);
                buckets_[bucket] = firstNode->next_;
                firstNode->next_ = tmp[new_bucket];
                tmp[new_bucket] = firstNode;
                firstNode = buckets_[bucket];
            }
        }

        buckets_.swap(tmp);
    }
}

template<class Key, class Value, class HashFn, class EqualKey>
void HashBase<Key, Value, HashFn, EqualKey>::erase(iterator pos)
{
    Node* pNode = pos.cur_;
    if (pNode) {
        const size_type n = buketIndex(pNode->val_);
        Node* pCurNode = buckets_[n];

        if (pCurNode == pNode) {
            buckets_[n] = pCurNode->next_;
            deleteNode(pCurNode);
            --numElements_;
        }
        else {
            Node* pNext = pCurNode->next_;
            while (pNext) {
                if (pNext == pNode) {
                    pCurNode->next_ = pNext->next_;
                    deleteNode(pNext);
                    --numElements_;
                    break;
                }
                else {
                    pCurNode = pNext;
                    pNext = pCurNode->next_;
                }
            }
        }
    }
}

template<class Key, class Value, class HashFn, class EqualKey>
typename HashBase<Key, Value, HashFn, EqualKey>::size_type
    HashBase<Key, Value, HashFn, EqualKey>::erase(const key_type& key)
{
    const size_type idx = bkt_num_key(key);
    Node* first = buckets_[idx];
    size_type erased = 0;

    if (first) {
        Node* cur = first;
        Node* next = cur->next_;
        while (next) {
            if (equals_(getKey(next->val_), key)) {
                cur->next_ = next->next_;
                deleteNode(next);
                next = cur->next_;
                ++erased;
                --numElements_;
            }
            else {
                cur = next;
                next = cur->next_;
            }
        }
        if (equals_(getKey(first->val_), key)) {
            buckets_[idx] = first->next_;
            deleteNode(first);
            ++erased;
            --numElements_;
        }
    }
    return erased;
}

// ===================== Iterators =====================

template<class Key, class Value, class HashFn, class EqualKey>
_HashBase_iterator<Key, Value, HashFn, EqualKey>&
    _HashBase_iterator<Key, Value, HashFn, EqualKey>::operator++()
{
    const Node* pOld = cur_;

    cur_ = cur_->next_;
    if (!cur_) {
        size_type buketIdx = hm_->buketIndex(pOld->val_);

#if X_ENABLE_ASSERTIONS
        // this detects issues in the hash function.
        // aka the hash is been created by a changing value like a pointer.
        if (pOld != hm_->buckets_[buketIdx]) {
            X_ASSERT_NOT_NULL(hm_->buckets_[buketIdx]);

            const Node* pTemp = hm_->buckets_[buketIdx]->next_;
            while (pTemp && pTemp != pOld) {
                pTemp = pTemp->next_;
            }

            X_ASSERT(pTemp != nullptr, "Index lookup error")();
        }
#endif // !X_ENABLE_ASSERTIONS

        while (!cur_ && ++buketIdx < hm_->buckets_.size()) {
            cur_ = hm_->buckets_[buketIdx];
        }
    }
    return *this;
}

template<class Key, class Value, class HashFn, class EqualKey>
inline _HashBase_iterator<Key, Value, HashFn, EqualKey>
    _HashBase_iterator<Key, Value, HashFn, EqualKey>::operator++(int)
{
    iterator tmp = *this;
    ++(*this); // call the function above.
    return tmp;
}

// Const

template<class Key, class Value, class HashFn, class EqualKey>
_HashBase_const_iterator<Key, Value, HashFn, EqualKey>&
    _HashBase_const_iterator<Key, Value, HashFn, EqualKey>::operator++()
{
    const Node* pOld = cur_;

    cur_ = cur_->next_;
    if (!cur_) {
        size_type buketIdx = hm_->buketIndex(pOld->val_);

#if X_ENABLE_ASSERTIONS
        // this detects issues in the hash function.
        // aka the hash is been created by a changing value like a pointer.
        if (pOld != hm_->buckets_[buketIdx]) {
            X_ASSERT_NOT_NULL(hm_->buckets_[buketIdx]);

            const Node* pTemp = hm_->buckets_[buketIdx]->next_;
            while (pTemp && pTemp != pOld) {
                pTemp = pTemp->next_;
            }

            X_ASSERT(pTemp != nullptr, "Index lookup error")();
        }
#endif // !X_ENABLE_ASSERTIONS

        while (!cur_ && ++buketIdx < hm_->buckets_.size()) {
            cur_ = hm_->buckets_[buketIdx];
        }
    }
    return *this;
}

template<class Key, class Value, class HashFn, class EqualKey>
inline _HashBase_const_iterator<Key, Value, HashFn, EqualKey>
    _HashBase_const_iterator<Key, Value, HashFn, EqualKey>::operator++(int)
{
    const_iterator tmp = *this;
    ++(*this); // call the function above.
    return tmp;
}

X_NAMESPACE_END

#endif // !_CON_HASH_BASE_H_
