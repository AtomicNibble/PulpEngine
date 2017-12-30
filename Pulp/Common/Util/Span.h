#pragma once

#include <array>     
#include <cstddef> 

#include <type_traits> // for enable_if_t, declval, is_convertible, inte...

X_DISABLE_WARNING(4814)

X_NAMESPACE_BEGIN(core)

constexpr const std::ptrdiff_t dynamic_extent = -1;

template <class ElementType, std::ptrdiff_t Extent = dynamic_extent>
class span;


namespace details
{
	template <class T>
	struct is_span_oracle : std::false_type
	{
	};

	template <class ElementType, std::ptrdiff_t Extent>
	struct is_span_oracle<span<ElementType, Extent>> : std::true_type
	{
	};

	template <class T>
	struct is_span : public is_span_oracle<std::remove_cv_t<T>>
	{
	};

	template <class T>
	struct is_std_array_oracle : std::false_type
	{
	};

	template <class ElementType, std::size_t Extent>
	struct is_std_array_oracle<std::array<ElementType, Extent>> : std::true_type
	{
	};

	template <class T>
	struct is_std_array : public is_std_array_oracle<std::remove_cv_t<T>>
	{
	};

	template <std::ptrdiff_t From, std::ptrdiff_t To>
	struct is_allowed_extent_conversion :
		public std::integral_constant<bool, From == To || From == dynamic_extent || To == dynamic_extent>
	{
	};

	template <class From, class To>
	struct is_allowed_element_type_conversion :
		public std::integral_constant<bool, std::is_convertible<From(*)[], To(*)[]>::value>
	{
	};

	template <class Span, bool IsConst>
	class span_iterator
	{
		using element_type_ = typename Span::element_type;

	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = std::remove_cv_t<element_type_>;
		using difference_type = typename Span::index_type;

		using reference = std::conditional_t<IsConst, const element_type_, element_type_>&;
		using pointer = std::add_pointer_t<reference>;

		span_iterator() = default;

		constexpr span_iterator(const Span* span, typename Span::index_type index) :
			span_(span), 
			index_(index)
		{
			X_ASSERT(span == nullptr || (0 <= index_ && index <= span_->length()), "")(span, index_, span_->length());
		}

		friend span_iterator<Span, true>;
		template<bool B, std::enable_if_t<!B && IsConst>* = nullptr>
		constexpr span_iterator(const span_iterator<Span, B>& other) :
			span_iterator(other.span_, other.index_)
		{
		}

		constexpr reference operator*() const 
		{
			X_ASSERT(index_ != span_->length(), "Out of bounds")(index_, span_->length());
			return *(span_->data() + index_);
		}

		constexpr pointer operator->() const 
		{
			X_ASSERT(index_ != span_->length(), "Out of bounds")(index_, span_->length());
			return span_->data() + index_;
		}

		constexpr span_iterator& operator++() 
		{
			X_ASSERT(0 <= index_ && index_ != span_->length(), "")(index_, span_->length());
			++index_;
			return *this;
		}

		constexpr span_iterator operator++(int) 
		{
			auto ret = *this;
			++(*this);
			return ret;
		}

		constexpr span_iterator& operator--() 
		{
			X_ASSERT(index_ != 0 && index_ <= span_->length(), "")(index_, span_->length());
			--index_;
			return *this;
		}

		constexpr span_iterator operator--(int) 
		{
			auto ret = *this;
			--(*this);
			return ret;
		}

		constexpr span_iterator operator+(difference_type n) const 
		{
			auto ret = *this;
			return ret += n;
		}

		constexpr span_iterator& operator+=(difference_type n) 
		{
			X_ASSERT((index_ + n) >= 0 && (index_ + n) <= span_->length(), "")();
			index_ += n;
			return *this;
		}

		constexpr span_iterator operator-(difference_type n) const 
		{
			auto ret = *this;
			return ret -= n;
		}

		constexpr span_iterator& operator-=(difference_type n) 
		{
			return *this += -n; 
		}

		constexpr difference_type operator-(const span_iterator& rhs) const 
		{
			X_ASSERT(span_ == rhs.span_, "")();
			return index_ - rhs.index_;
		}

		constexpr reference operator[](difference_type n) const 
		{
			return *(*this + n);
		}

		constexpr friend bool operator==(const span_iterator& lhs,
			const span_iterator& rhs) 
		{
			return lhs.span_ == rhs.span_ && lhs.index_ == rhs.index_;
		}

		constexpr friend bool operator!=(const span_iterator& lhs,
			const span_iterator& rhs) 
		{
			return !(lhs == rhs);
		}

		constexpr friend bool operator<(const span_iterator& lhs,
			const span_iterator& rhs) 
		{
			X_ASSERT(lhs.span_ == rhs.span_, "")();
			return lhs.index_ < rhs.index_;
		}

		constexpr friend bool operator<=(const span_iterator& lhs,
			const span_iterator& rhs) 
		{
			return !(rhs < lhs);
		}

		constexpr friend bool operator>(const span_iterator& lhs,
			const span_iterator& rhs) 
		{
			return rhs < lhs;
		}

		constexpr friend bool operator>=(const span_iterator& lhs,
			const span_iterator& rhs) 
		{
			return !(rhs > lhs);
		}

	protected:
		const Span* span_ = nullptr;
		std::ptrdiff_t index_ = 0;
	};

	template <class Span, bool IsConst>
	inline constexpr span_iterator<Span, IsConst>
		operator+(typename span_iterator<Span, IsConst>::difference_type n,
			const span_iterator<Span, IsConst>& rhs) 
	{
		return rhs + n;
	}

	template <class Span, bool IsConst>
	inline constexpr span_iterator<Span, IsConst>
		operator-(typename span_iterator<Span, IsConst>::difference_type n,
			const span_iterator<Span, IsConst>& rhs) 
	{
		return rhs - n;
	}

	template <std::ptrdiff_t Ext>
	class extent_type
	{
	public:
		using index_type = std::ptrdiff_t;

		static_assert(Ext >= 0, "A fixed-size span must be >= 0 in size.");

		constexpr extent_type() 
		{
		}

		template <index_type Other>
		constexpr extent_type(extent_type<Other> ext)
		{
			static_assert(Other == Ext || Other == dynamic_extent,
				"Mismatch between fixed-size extent and size of initializing data.");
			X_ASSERT(ext.size() == Ext,"")(ext.size(), Ext);
		}

		constexpr extent_type(index_type size) {
			X_ASSERT(size == Ext, "Mismatch between fixed-size extent and size of initializing data.")(size, Ext);
		}

		constexpr index_type size(void) const {
			return Ext; 
		}
	};

	template <>
	class extent_type<dynamic_extent>
	{
	public:
		using index_type = std::ptrdiff_t;

		template <index_type Other>
		explicit constexpr extent_type(extent_type<Other> ext) : 
			size_(ext.size())
		{
		}

		explicit constexpr extent_type(index_type size) : 
			size_(size) 
		{
			X_ASSERT(size >= 0, "Extent size can't be negative")(size); 
		}

		constexpr index_type size(void) const { 
			return size_; 
		}

	private:
		index_type size_;
	};

} // namespace details



template <class ElementType, std::ptrdiff_t Extent>
class span
{
public:
	// constants and types
	using element_type = ElementType;
	using value_type = std::remove_cv_t<ElementType>;
	using index_type = std::ptrdiff_t;
	using pointer = element_type*;
	using reference = element_type&;

	using iterator = details::span_iterator<span<ElementType, Extent>, false>;
	using const_iterator = details::span_iterator<span<ElementType, Extent>, true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	using size_type = index_type;

	constexpr static const index_type extent = Extent;

private:
	template <class ExtentType>
	class storage_type : public ExtentType
	{
	public:

		// checked parameter is needed to remove unnecessary null check in subspans
		template <class OtherExtentType>
		constexpr storage_type(pointer data, OtherExtentType ext, bool checked = false) : ExtentType(ext), data_(data)
		{
			X_ASSERT(((checked || !data) && ExtentType::size() == 0) ||
				((checked || data) && ExtentType::size() >= 0), "")(checked, ext, data);
		}

		constexpr pointer data(void) const { return data_; }

	private:
		pointer data_;
	};

	constexpr span(pointer ptr, index_type count, bool checked) :
		storage_(ptr, count, checked)
	{
	}

public:

	// [span.cons], span constructors, copy, assignment, and destructor
		// "Dependent" is needed to make "std::enable_if_t<Dependent || Extent <= 0>" SFINAE,
		// since "std::enable_if_t<Extent <= 0>" is ill-formed when Extent is greater than 0.
	template <bool Dependent = false, 
		class = std::enable_if_t<(Dependent || Extent <= 0)>> constexpr span() : 
			storage_(nullptr, details::extent_type<0>())
	{
	}

	constexpr span(std::nullptr_t) : 
		span() 
	{
	}

	constexpr span(pointer ptr, index_type count) : 
		storage_(ptr, count) 
	{
	}

	constexpr span(pointer firstElem, pointer lastElem) :
		storage_(firstElem, std::distance(firstElem, lastElem))
	{
	}

	template <std::size_t N>
	constexpr span(element_type(&arr)[N]) :
		storage_(&arr[0], details::extent_type<N>())
	{
	}

	template <std::size_t N, class ArrayElementType = std::remove_const_t<element_type>>
	constexpr span(std::array<ArrayElementType, N>& arr) :
		storage_(&arr[0], details::extent_type<N>())
	{
	}

	template <std::size_t N>
	constexpr span(const std::array<std::remove_const_t<element_type>, N>& arr) :
		storage_(&arr[0], details::extent_type<N>())
	{
	}

	// NB: the SFINAE here uses .data() as a incomplete/imperfect proxy for the requirement
	// on Container to be a contiguous sequence container.
	template <class Container,
		class = std::enable_if_t<
		!details::is_span<Container>::value && !details::is_std_array<Container>::value &&
		std::is_convertible<typename Container::pointer, pointer>::value &&
		std::is_convertible<typename Container::pointer,
		decltype(std::declval<Container>().data())>::value>>
		constexpr span(Container& cont) : 
			span(cont.data(), safe_static_cast<index_type>(cont.size()))
	{
	}

	template <class Container,
		class = std::enable_if_t<
		std::is_const<element_type>::value && !details::is_span<Container>::value &&
		std::is_convertible<typename Container::pointer, pointer>::value &&
		std::is_convertible<typename Container::pointer,
		decltype(std::declval<Container>().data())>::value>>
		constexpr span(const Container& cont) : 
			span(cont.data(), safe_static_cast<index_type>(cont.size()))
	{
	}

	constexpr span(const span& other) = default;
	constexpr span(span&& other) = default;

	template <
		class OtherElementType, std::ptrdiff_t OtherExtent,
		class = std::enable_if_t<
		details::is_allowed_extent_conversion<OtherExtent, Extent>::value &&
		details::is_allowed_element_type_conversion<OtherElementType, element_type>::value>>
		constexpr span(const span<OtherElementType, OtherExtent>& other) :
			storage_(other.data(), details::extent_type<OtherExtent>(other.size()))
	{
	}

	template <
		class OtherElementType, std::ptrdiff_t OtherExtent,
		class = std::enable_if_t<
		details::is_allowed_extent_conversion<OtherExtent, Extent>::value &&
		details::is_allowed_element_type_conversion<OtherElementType, element_type>::value>>
		constexpr span(span<OtherElementType, OtherExtent>&& other) :
			storage_(other.data(), details::extent_type<OtherExtent>(other.size()))
	{
	}

	~span() = default;

	span& operator=(const span& other) = default;
	span& operator=(span&& other) = default;

	// [span.sub], span subviews
	template <std::ptrdiff_t Count>
	constexpr span<element_type, Count> first() const
	{
		X_ASSERT(Count >= 0 && Count <= size(), "Count out of bounds")(Count, size());
		return{ data(), Count };
	}

	template <std::ptrdiff_t Count>
	constexpr span<element_type, Count> last() const
	{
		X_ASSERT(Count >= 0 && size() - Count >= 0, "Count out of bounds")(Count, size(), size() - Count);
		return{ data() + (size() - Count), Count };
	}

	template <std::ptrdiff_t Offset, std::ptrdiff_t Count = dynamic_extent>
	constexpr span<element_type, Count> subspan() const
	{
		X_ASSERT((Offset >= 0 && size() - Offset >= 0) &&
			(Count == dynamic_extent || (Count >= 0 && Offset + Count <= size())), "Invalid range")(Count, Offset, size());

		return{ data() + Offset, Count == dynamic_extent ? size() - Offset : Count };
	}

	constexpr span<element_type, dynamic_extent> first(index_type count) const
	{
		X_ASSERT(count >= 0 && count <= size(), "Out of bounds")(count, size());
		return{ data(), count };
	}

	constexpr span<element_type, dynamic_extent> last(index_type count) const
	{
		return make_subspan(size() - count, dynamic_extent, subspan_selector<Extent>{});
	}

	constexpr span<element_type, dynamic_extent> subspan(index_type offset,
		index_type count = dynamic_extent) const
	{
		return make_subspan(offset, count, subspan_selector<Extent>{});
	}


	// [span.obs], span observers
	constexpr index_type length(void) const { return size(); }
	constexpr index_type size(void) const { return storage_.size(); }
	constexpr index_type length_bytes(void) const { return size_bytes(); }
	constexpr index_type size_bytes(void) const
	{
		return size() * narrow_cast<index_type>(sizeof(element_type));
	}
	constexpr bool empty(void) const { return size() == 0; }

	// [span.elem], span element access
	constexpr reference operator[](index_type idx) const
	{
		X_ASSERT(idx >= 0 && idx < storage_.size(), "Out of bounds")(idx, storage_.size());
		return data()[idx];
	}

	constexpr reference at(index_type idx) const { return this->operator[](idx); }
	constexpr reference operator()(index_type idx) const { return this->operator[](idx); }
	constexpr pointer data(void) const { return storage_.data(); }

	iterator begin() const { return{ this, 0 }; }
	iterator end() const { return{ this, length() }; }

	const_iterator cbegin() const { return{ this, 0 }; }
	const_iterator cend() const { return{ this, length() }; }

	reverse_iterator rbegin() const { return reverse_iterator{ end() }; }
	reverse_iterator rend() const { return reverse_iterator{ begin() }; }

	const_reverse_iterator crbegin() const { return const_reverse_iterator{ cend() }; }
	const_reverse_iterator crend() const { return const_reverse_iterator{ cbegin() }; }

private:

	template <std::ptrdiff_t CallerExtent>
	class subspan_selector {};

	template <std::ptrdiff_t CallerExtent>
	span<element_type, dynamic_extent> make_subspan(index_type offset, index_type count,
		subspan_selector<CallerExtent>) const
	{
		span<element_type, dynamic_extent> tmp(*this);
		return tmp.subspan(offset, count);
	}

	span<element_type, dynamic_extent> make_subspan(index_type offset, index_type count,
		subspan_selector<dynamic_extent>) const
	{
		X_ASSERT(offset >= 0 && size() - offset >= 0, "")(offset, size());
		if (count == dynamic_extent)
		{
			return{ data() + offset, size() - offset, true };
		}

		X_ASSERT(count >= 0 && size() - offset >= count, "")(count, size());
		return{ data() + offset,  count, true };
	}

private:
	storage_type<details::extent_type<Extent>> storage_;
};


// [span.comparison], span comparison operators
template <class ElementType, std::ptrdiff_t FirstExtent, std::ptrdiff_t SecondExtent>
inline constexpr bool operator==(const span<ElementType, FirstExtent>& l,
	const span<ElementType, SecondExtent>& r)
{
	return std::equal(l.begin(), l.end(), r.begin(), r.end());
}

template <class ElementType, std::ptrdiff_t Extent>
inline constexpr bool operator!=(const span<ElementType, Extent>& l,
	const span<ElementType, Extent>& r)
{
	return !(l == r);
}

template <class ElementType, std::ptrdiff_t Extent>
inline constexpr bool operator<(const span<ElementType, Extent>& l,
	const span<ElementType, Extent>& r)
{
	return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
}

template <class ElementType, std::ptrdiff_t Extent>
inline constexpr bool operator<=(const span<ElementType, Extent>& l,
	const span<ElementType, Extent>& r)
{
	return !(l > r);
}

template <class ElementType, std::ptrdiff_t Extent>
inline constexpr bool operator>(const span<ElementType, Extent>& l,
	const span<ElementType, Extent>& r)
{
	return r < l;
}

template <class ElementType, std::ptrdiff_t Extent>
inline constexpr bool operator>=(const span<ElementType, Extent>& l,
	const span<ElementType, Extent>& r)
{
	return !(l < r);
}


//
// make_span() - Utility functions for creating spans
//
template <class ElementType>
span<ElementType> make_span(ElementType* ptr, typename span<ElementType>::index_type count)
{
	return span<ElementType>(ptr, count);
}

template <class ElementType>
span<ElementType> make_span(ElementType* firstElem, ElementType* lastElem)
{
	return span<ElementType>(firstElem, lastElem);
}

template <class ElementType, std::size_t N>
span<ElementType, N> make_span(ElementType(&arr)[N])
{
	return span<ElementType, N>(arr);
}

template <class Container>
span<typename Container::value_type> make_span(Container& cont)
{
	return span<typename Container::value_type>(cont);
}

template <class Container>
span<const typename Container::value_type> make_span(const Container& cont)
{
	return span<const typename Container::value_type>(cont);
}

template <class Ptr>
span<typename Ptr::element_type> make_span(Ptr& cont, std::ptrdiff_t count)
{
	return span<typename Ptr::element_type>(cont, count);
}

template <class Ptr>
span<typename Ptr::element_type> make_span(Ptr& cont)
{
	return span<typename Ptr::element_type>(cont);
}


X_ENABLE_WARNING(4814)

X_NAMESPACE_END