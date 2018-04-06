
X_NAMESPACE_BEGIN(net)

template<typename T>
RangeList<T>::RangeList(core::MemoryArenaBase* arena) :
    ranges_(arena)
{
}

template<typename T>
size_t RangeList<T>::writeToBitStream(core::FixedBitStreamBase& bs, BitSizeT maxBits, bool removeAdded)
{
    bs.alignWriteToByteBoundry(); // make sure we write on boundry.

    // make sure we won't end up negative.
    if (maxBits < bs.size() + sizeof(uint16_t)) {
        return 0;
    }

    const size_t spaceLeft = maxBits - (bs.size() + sizeof(uint16_t));
    const size_t numFit = core::Min(spaceLeft / core::bitUtil::bytesToBits(sizeof(RangeNode)), ranges_.size());
    if (!numFit) {
        return 0;
    }

    bs.write(safe_static_cast<uint16_t>(numFit));
    bs.write<RangeNode>(ranges_.data(), numFit);

    if (removeAdded) {
        std::rotate(ranges_.begin(), ranges_.begin() + numFit, ranges_.end());
        ranges_.resize(ranges_.size() - numFit);
    }

    return numFit;
}

template<typename T>
bool RangeList<T>::fromBitStream(core::FixedBitStreamBase& bs)
{
    clear();

    // meow
    uint16_t num;
    bs.readAligned(num);

    size_t bitsRequired = num * core::bitUtil::bytesToBits(sizeof(RangeNode));
    if (bitsRequired < bs.size()) {
        return false;
    }

    ranges_.resize(num);

    bs.read<RangeNode>(ranges_.data(), num);
    return true;
}

template<typename T>
void RangeList<T>::add(RangeType val)
{
    // find a range to expand / potentially merge ranges if it joins them.
    if (ranges_.isEmpty()) {
        ranges_.emplace_back(val, val);
        return;
    }

    // we want to find the first node that we can merge into.
    // in order to merge into a list the min / max needs to be off by one.
    // we also need to correctly select the range that mathes
    auto compare = [](const typename RangeArr::Type& range, RangeType val) -> size_t {
        // lower?
        if (val < range.min - 1) {
            return -1;
        }
        // higer?
        if (val > range.max + 1) {
            return 1;
        }

        // add me.
        return 0;
    };

    int32_t min = 0;
    int32_t max = safe_static_cast<int32_t>(ranges_.size() - 1);
    int32_t rangeIdx = safe_static_cast<int32_t>(ranges_.size());
    while (min <= max) {
        const auto idx = (min + max) / 2;
        const auto& range = ranges_[idx];

        auto res = compare(range, val);
        if (res == 0) {
            rangeIdx = idx;
            break;
        }
        else if (res > 0) {
            min = idx + 1;
        }
        else {
            max = idx - 1;
        }
    }

    const T one = T(1);

    // not found
    if (rangeIdx == ranges_.size()) {
        // extend end if possible.
        if (ranges_[rangeIdx - 1].max + one == val) {
            ranges_[rangeIdx - 1].max++;
        }
        else {
            ranges_.emplace_back(val, val);
        }
        return;
    }

    auto& range = ranges_[rangeIdx];

    if (val < range.min - one) {
        ranges_.insertAtIndex(rangeIdx, typename RangeArr::Type(val, val));
        return;
    }
    else if (val == range.min - one) {
        --range.min;
        // merge?
        if (rangeIdx > 0 && (ranges_[rangeIdx - 1].max + one) == range.min) {
            ranges_[rangeIdx - 1].max = range.max;
            ranges_.removeIndex(rangeIdx);
        }

        return;
    }
    else if (val >= range.min && val <= range.max) {
        return;
    }
    else if (val == range.max + one) {
        ++range.max;
        // merge?
        if ((rangeIdx + 1) < safe_static_cast<int32_t>(ranges_.size()) && ranges_[rangeIdx + 1].min == range.max + one) {
            ranges_[rangeIdx].max = ranges_[rangeIdx + 1].max;
            ranges_.removeIndex(rangeIdx + 1);
        }

        return;
    }

    X_ASSERT_UNREACHABLE();
}

template<typename T>
X_INLINE const T& RangeList<T>::operator[](typename RangeArr::size_type idx) const
{
    return ranges_[idx];
}

template<typename T>
X_INLINE T& RangeList<T>::operator[](typename RangeArr::size_type idx)
{
    return ranges_[idx];
}

template<typename T>
X_INLINE const bool RangeList<T>::isEmpty(void) const
{
    return ranges_.isEmpty();
}

template<typename T>
X_INLINE const bool RangeList<T>::isNotEmpty(void) const
{
    return ranges_.isNotEmpty();
}

template<typename T>
X_INLINE void RangeList<T>::reserve(typename RangeArr::size_type size)
{
    ranges_.reserve(size);
}

template<typename T>
X_INLINE void RangeList<T>::clear(void)
{
    ranges_.clear();
}

template<typename T>
X_INLINE void RangeList<T>::free(void)
{
    ranges_.free();
}

template<typename T>
X_INLINE typename RangeList<T>::size_type RangeList<T>::size(void) const
{
    return ranges_.size();
}

template<typename T>
X_INLINE typename RangeList<T>::size_type RangeList<T>::capacity(void) const
{
    return ranges_.capacity();
}

template<typename T>
X_INLINE typename RangeList<T>::Iterator RangeList<T>::begin(void)
{
    return ranges_.begin();
}

template<typename T>
X_INLINE typename RangeList<T>::ConstIterator RangeList<T>::begin(void) const
{
    return ranges_.begin();
}

template<typename T>
X_INLINE typename RangeList<T>::Iterator RangeList<T>::end(void)
{
    return ranges_.end();
}

template<typename T>
X_INLINE typename RangeList<T>::ConstIterator RangeList<T>::end(void) const
{
    return ranges_.end();
}

template<typename T>
X_INLINE typename RangeList<T>::Reference RangeList<T>::front(void)
{
    return ranges_.front();
}

template<typename T>
X_INLINE typename RangeList<T>::ConstReference RangeList<T>::front(void) const
{
    return ranges_.front();
}

template<typename T>
X_INLINE typename RangeList<T>::Reference RangeList<T>::back(void)
{
    return ranges_.back();
}

template<typename T>
X_INLINE typename RangeList<T>::ConstReference RangeList<T>::back(void) const
{
    return ranges_.back();
}

X_NAMESPACE_END