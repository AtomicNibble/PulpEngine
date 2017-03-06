
X_NAMESPACE_BEGIN(net)



template<typename T>
RangeList<T>::RangeList(core::MemoryArenaBase* arena) :
	ranges_(arena)
{

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
	auto compare = [](const RangeArr::Type& range, RangeType val) -> size_t {

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
	while (min <= max)
	{
		const auto idx = (min + max) / 2;
		const auto& range = ranges_[idx];

		auto res = compare(range, val);
		if (res == 0)
		{
			rangeIdx = idx;
			break;
		}
		else if (res > 0)
		{
			min = idx + 1;
		}
		else
		{
			max = idx - 1;
		}
	}

	// not found
	if (rangeIdx == ranges_.size())
	{
		// extend end if possible.
		if (ranges_[rangeIdx - 1].max + 1 == val) {
			ranges_[rangeIdx - 1].max++;
		}
		else {
			ranges_.emplace_back(val, val);
		}
		return;
	}

	auto& range = ranges_[rangeIdx];

	if (val < range.min - 1)
	{
		ranges_.insert(RangeArr::Type(val, val), rangeIdx);
		return;
	}
	else if (val == range.min - 1)
	{
		--range.min;
		// merge?
		if (rangeIdx > 0 && (ranges_[rangeIdx - 1].max + 1) == range.min) {
			ranges_[rangeIdx - 1].max = range.max;
			ranges_.removeIndex(rangeIdx);
		}

		return;
	}
	else if (val >= range.min && val <= range.max)
	{
		return;
	}
	else if (val == range.max + 1)
	{
		++range.max;
		// merge?
		if ((rangeIdx + 1) < ranges_.size() && ranges_[rangeIdx + 1].min == range.max + 1) {
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
X_INLINE typename RangeList<T>::RangeArr::size_type RangeList<T>::size(void)
{
	return ranges_.size();
}

template<typename T>
X_INLINE typename RangeList<T>::RangeArr::size_type RangeList<T>::capacity(void)
{
	return ranges_.capacity();
}

X_NAMESPACE_END