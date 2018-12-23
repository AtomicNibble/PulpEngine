
X_NAMESPACE_BEGIN(core)



template<typename Key, typename Value, class Hash, class KeyEqual>
inline FixedHashTableOwningPolicy<Key, Value, Hash, KeyEqual>::FixedHashTableOwningPolicy(core::MemoryArenaBase* arena, size_type maxItem) :
    FixedHashTableBase<Key, Value, Hash, KeyEqual>(reinterpret_cast<BaseT::value_type*>(
        X_NEW_ARRAY_ALIGNED(uint8_t, maxItem * sizeof(BaseT::value_type), arena, "FixedHashTableOwningPolicy", X_ALIGN_OF(BaseT::value_type))), maxItem
    ),
    arena_(arena)
{
}

template<typename Key, typename Value, class Hash, class KeyEqual>
inline FixedHashTableOwningPolicy<Key, Value, Hash, KeyEqual>::~FixedHashTableOwningPolicy()
{
    X_DELETE_ARRAY(pData_, arena_);
}

template<size_t N, typename Key, typename Value, class Hash, class KeyEqual>
inline FixedHashTableStackPolicy<N, Key, Value, Hash, KeyEqual>::FixedHashTableStackPolicy() :
    FixedHashTableBase<Key, Value, Hash, KeyEqual>(reinterpret_cast<BaseT::value_type*>(array_), N)
{
}

// ---------------------------------------------------------------------

template<typename Key, typename Value, class Hash, class KeyEqual>
FixedHashTableBase<Key, Value, Hash, KeyEqual>::FixedHashTableBase(FixedHashTableBase&& oth) :
    pData_(oth.pData_),
    num_(oth.num_),
    mask_(oth.mask_),
    size_(oth.size_)
{
    oth.pData_ = nullptr;
    oth.size_ = 0;

    std::memset(emptyEntry_, -1, sizeof(emptyEntry_));
}


template<typename Key, typename Value, class Hash, class KeyEqual>
FixedHashTableBase<Key, Value, Hash, KeyEqual>::FixedHashTableBase(value_type* pData, size_type maxItems) :
    pData_(X_ASSERT_NOT_NULL(pData)),
    num_(maxItems),
    mask_(maxItems - 1),
    size_(0)
{
    X_ASSERT(bitUtil::IsPowerOfTwo(maxItems), "maxItems must be pow2")(maxItems);

    // set to -1
    std::memset(emptyEntry_, -1, sizeof(emptyEntry_));
    std::memset(pData_, -1, num_ * sizeof(value_type));
}

template<typename Key, typename Value, class Hash, class KeyEqual>
FixedHashTableBase<Key, Value, Hash, KeyEqual>::~FixedHashTableBase() 
{
    clear();
}

template<typename Key, typename Value, class Hash, class KeyEqual>
void FixedHashTableBase<Key, Value, Hash, KeyEqual>::clear(void)
{
    // only times this should be null is if we where moved.
    if (!pData_) {
        return;
    }

    size_type idx = 0;

    while (size_ > 0)
    {
        // find next one yo.
        while (isIndexEmpty(idx)) {
            ++idx;
        }

        // DELETE ME!
        destroyIndex(idx);
        --size_;
    }

#if X_ENABLE_ASSERTIONS 

    for (size_type i = 0; i < num_; i++)
    {
        X_ASSERT(isIndexEmpty(i), "Index is not empty")(i);
    }

#endif // !X_ENABLE_ASSERTIONS
}


template<typename Key, typename Value, class Hash, class KeyEqual>
template <class... Args>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::return_pair FixedHashTableBase<Key, Value, Hash, KeyEqual>::emplace(Args&&... args)
{
    return emplace_impl(std::forward<Args>(args)...);
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::return_pair FixedHashTableBase<Key, Value, Hash, KeyEqual>::insert(const value_type& val)
{
    return emplace_impl(val.first, val.second);
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::return_pair FixedHashTableBase<Key, Value, Hash, KeyEqual>::insert(value_type&& val)
{
    return emplace_impl(std::move(val.first), std::move(val.second));
}


template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::find(const key_type& key)
{
    return find_impl(key);
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::const_iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::find(const key_type& key) const
{
    return find_impl(key);
}


template<typename Key, typename Value, class Hash, class KeyEqual>
template <typename K>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::find(const K& x)
{
    return find_impl(x);
}

template<typename Key, typename Value, class Hash, class KeyEqual>
template <typename K>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::const_iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::find(const K& x) const
{
    return find_impl(x);
}

template<typename Key, typename Value, class Hash, class KeyEqual>
void FixedHashTableBase<Key, Value, Hash, KeyEqual>::erase(iterator it)
{
    erase_impl(it);
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::size_type FixedHashTableBase<Key, Value, Hash, KeyEqual>::erase(const key_type& key)
{
    return erase_impl(key);
}

template<typename Key, typename Value, class Hash, class KeyEqual>
template <typename K>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::size_type FixedHashTableBase<Key, Value, Hash, KeyEqual>::erase(const K& x)
{
    return erase_impl(x);
}


template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::mapped_type& FixedHashTableBase<Key, Value, Hash, KeyEqual>::operator[] (const key_type& key)
{
    return emplace_impl(key).first->second;
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::mapped_type& FixedHashTableBase<Key, Value, Hash, KeyEqual>::operator[] (key_type&& key)
{
    return emplace_impl(std::forward<key_type>(key)).first->second;
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::size_type FixedHashTableBase<Key, Value, Hash, KeyEqual>::size(void) const
{
    return size_;
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::size_type FixedHashTableBase<Key, Value, Hash, KeyEqual>::capacity(void) const
{
    return num_;
}

template<typename Key, typename Value, class Hash, class KeyEqual>
bool FixedHashTableBase<Key, Value, Hash, KeyEqual>::isEmpty(void) const
{
    return size() == 0;
}

template<typename Key, typename Value, class Hash, class KeyEqual>
bool FixedHashTableBase<Key, Value, Hash, KeyEqual>::isNotEmpty(void) const
{
    return size() > 0;
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::begin()
{ 
    return iterator(this); 
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::const_iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::begin() const
{ 
    return const_iterator(this); 
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::end()
{ 
    return iterator(this, num_); 
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::const_iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::end() const
{ 
    return const_iterator(this, num_); 
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::const_iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::cbegin() const
{ 
    return const_iterator(this); 
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::const_iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::cend() const
{ 
    return const_iterator(this, num_); 
}

template<typename Key, typename Value, class Hash, class KeyEqual>
template <typename K, typename... Args>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::return_pair FixedHashTableBase<Key, Value, Hash, KeyEqual>::emplace_impl(const K& key, Args&&... args)
{
    for (size_type idx = key2idx(key); ; idx = probeNext(idx))
    {
        if (isIndexEmpty(idx)) {
            if constexpr (sizeof...(args) == 0) {
                Mem::Construct<value_type>(&pData_[idx], key, Value());
            }
            else {
                Mem::Construct<value_type>(&pData_[idx], key, std::forward<Args>(args)...);
            }
            size_++;
            return { iterator(this, idx), true };
        }
        else if (key_equal()(pData_[idx].first, key)) {
            return { iterator(this, idx), false };
        }
    }
}

template<typename Key, typename Value, class Hash, class KeyEqual>
template <typename K, typename... Args>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::return_pair FixedHashTableBase<Key, Value, Hash, KeyEqual>::emplace_impl(K&& key, Args&&... args)
{
    for (size_type idx = key2idx(key); ; idx = probeNext(idx))
    {
        if (isIndexEmpty(idx)) {
            if constexpr (sizeof...(args) == 0) {
                Mem::Construct<value_type>(&pData_[idx], std::forward<K>(key), Value());
            }
            else {
                Mem::Construct<value_type>(&pData_[idx], std::forward<K>(key), std::forward<Args>(args)...);
            }
            size_++;
            return { iterator(this, idx), true };
        }
        else if (key_equal()(pData_[idx].first, key)) {
            return { iterator(this, idx), false };
        }
    }
}

template<typename Key, typename Value, class Hash, class KeyEqual>
template <typename K>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::find_impl(const K& key)
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

template<typename Key, typename Value, class Hash, class KeyEqual>
template <typename K>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::const_iterator FixedHashTableBase<Key, Value, Hash, KeyEqual>::find_impl(const K& key) const
{
    for (size_type idx = key2idx(key); ; idx = probeNext(idx))
    {
        if (isIndexEmpty(idx)) {
            return end();
        }
        if (key_equal()(pData_[idx].first, key)) {
            return const_iterator(this, idx);
        }
    }
}

template<typename Key, typename Value, class Hash, class KeyEqual>
void FixedHashTableBase<Key, Value, Hash, KeyEqual>::erase_impl(iterator it)
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
            // TODO: deconstruct?
            X_ASSERT(!isIndexEmpty(bucket) && !isIndexEmpty(idx), "Potential leak, fix me")(isIndexEmpty(bucket), isIndexEmpty(idx));
            pData_[bucket] = pData_[idx];
            bucket = idx;
        }
    }
}

template<typename Key, typename Value, class Hash, class KeyEqual>
template <typename K>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::size_type FixedHashTableBase<Key, Value, Hash, KeyEqual>::erase_impl(const K &key) 
{
    auto it = find_impl(key);
    if (it != end()) {
        erase_impl(it);
        return 1;
    }
    return 0;
}


template<typename Key, typename Value, class Hash, class KeyEqual>
template <typename K>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::size_type FixedHashTableBase<Key, Value, Hash, KeyEqual>::key2idx(const K& key) const
{
    return hasher()(key) & mask_;
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::size_type FixedHashTableBase<Key, Value, Hash, KeyEqual>::probeNext(size_type idx) const
{
    return (idx + 1) & mask_;
}

template<typename Key, typename Value, class Hash, class KeyEqual>
typename FixedHashTableBase<Key, Value, Hash, KeyEqual>::size_type FixedHashTableBase<Key, Value, Hash, KeyEqual>::diff(size_type a, size_type b) const
{
    return (num_ + (a - b)) & mask_;
}

template<typename Key, typename Value, class Hash, class KeyEqual>
bool FixedHashTableBase<Key, Value, Hash, KeyEqual>::isIndexEmpty(size_type idx) const 
{
    return std::memcmp(&pData_[idx], &emptyEntry_, sizeof(emptyEntry_)) == 0;
}

template<typename Key, typename Value, class Hash, class KeyEqual>
void FixedHashTableBase<Key, Value, Hash, KeyEqual>::destroyIndex(size_type idx) 
{
    Mem::Destruct<value_type>(&pData_[idx]);
    std::memset(&pData_[idx], -1, sizeof(value_type));
}



X_NAMESPACE_END
