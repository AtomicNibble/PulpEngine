#pragma once


#ifndef X_CONTAINER_FIXED_RING_BUFFER_H_
#define X_CONTAINER_FIXED_RING_BUFFER_H_


#include <iterator>

X_NAMESPACE_BEGIN(core)

template <
typename T,                        //circular_buffer type
typename T_nonconst,                         //with any consts
typename elem_type = typename T::value_type> //+ const for const iter
class circular_buffer_iterator
{
public:

	typedef circular_buffer_iterator<T, T_nonconst, elem_type> self_type;

	typedef T                                   cbuf_type;
	typedef std::random_access_iterator_tag     iterator_category;
	typedef typename cbuf_type::value_type      value_type;
	typedef typename cbuf_type::size_type       size_type;
	typedef typename cbuf_type::pointer         pointer;
	typedef typename cbuf_type::const_pointer   const_pointer;
	typedef typename cbuf_type::reference       reference;
	typedef typename cbuf_type::const_reference const_reference;
	typedef typename cbuf_type::difference_type difference_type;

	circular_buffer_iterator(cbuf_type* b, size_type p) : 
		buf_(b),
		pos_(p) 
	{
	}

	// Converting a non-const iterator to a const iterator
	circular_buffer_iterator(const circular_buffer_iterator<T_nonconst, T_nonconst,
		typename T_nonconst::value_type>& other) : 
		buf_(other.buf_),
		pos_(other.pos_)
	{
	}

	friend class circular_buffer_iterator<const T, T, const elem_type>;

	// Use compiler generated copy ctor, copy assignment operator and dtor

	elem_type &operator*()  { return (*buf_)[pos_]; }
	elem_type *operator->() { return &(operator*()); }

	self_type &operator++()
	{
		pos_ += 1;
		return *this;
	}
	self_type operator++(int)
	{
		self_type tmp(*this);
		++(*this);
		return tmp;
	}

	self_type &operator--()
	{
		pos_ -= 1;
		return *this;
	}
	self_type operator--(int)
	{
		self_type tmp(*this);
		--(*this);
		return tmp;
	}

	self_type operator+(difference_type n) const
	{
		self_type tmp(*this);
		tmp.pos_ += n;
		return tmp;
	}
	self_type &operator+=(difference_type n)
	{
		pos_ += n;
		return *this;
	}

	self_type operator-(difference_type n) const
	{
		self_type tmp(*this);
		tmp.pos_ -= n;
		return tmp;
	}
	self_type &operator-=(difference_type n)
	{
		pos_ -= n;
		return *this;
	}

	difference_type operator-(const self_type &c) const
	{
		return pos_ - c.pos_;
	}

	bool operator==(const self_type &other) const
	{
		return pos_ == other.pos_ && buf_ == other.buf_;
	}
	bool operator!=(const self_type &other) const
	{
		return pos_ != other.pos_ && buf_ == other.buf_;
	}
	bool operator>(const self_type &other) const
	{
		return pos_ > other.pos_;
	}
	bool operator>=(const self_type &other) const
	{
		return pos_ >= other.pos_;
	}
	bool operator<(const self_type &other) const
	{
		return pos_ < other.pos_;
	}
	bool operator<=(const self_type &other) const
	{
		return pos_ <= other.pos_;
	}

private:
	cbuf_type* buf_;
	size_type  pos_;
};

template <typename circular_buffer_iterator_t>
circular_buffer_iterator_t operator+
(const typename circular_buffer_iterator_t::difference_type &a,
const circular_buffer_iterator_t                           &b)
{
	return circular_buffer_iterator_t(a) + b;
}

template <typename circular_buffer_iterator_t>
circular_buffer_iterator_t operator-
(const typename circular_buffer_iterator_t::difference_type &a,
const circular_buffer_iterator_t                           &b)
{
	return circular_buffer_iterator_t(a) - b;
}





template<typename T, size_t N>
class FixedRingBuffer
{
public:
	typedef FixedRingBuffer<T, N> self;
	typedef T			value_type;
	typedef T*          pointer;
	typedef const T*    const_pointer;
	typedef T&			reference;
	typedef const T&	const_reference;
	typedef T			difference_type;

	typedef size_t		size_type;

	typedef circular_buffer_iterator<self, self> iterator;
	typedef circular_buffer_iterator<const self, self, const T> const_iterator;
	
	typedef std::reverse_iterator<iterator>       reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

public:
	FixedRingBuffer();
	~FixedRingBuffer();

	void clear(void);

	size_type size(void) const;
	size_type capacity(void) const;

	void append(const T& val);
	void push_back(const T& val);


	iterator         begin();
	iterator         end();

	const_iterator   begin() const;
	const_iterator   end() const;

	reverse_iterator rbegin();
	reverse_iterator rend();

	const_reverse_iterator rbegin() const;
	const_reverse_iterator rend() const;


	reference       operator[](size_type idx);
	const_reference operator[](size_type idx) const;

protected:
	size_type normalise(size_type idx) const;
	size_type index_to_subscript(size_type idx) const;

	size_type next_tail();

	void increment_tail();
	void increment_head();

protected:
	size_type num_;
	size_type head_;
	size_type tail_;

	T array_[N];
};


X_NAMESPACE_END

#include "FixedRingBuffer.inl"


#endif // !X_CONTAINER_FIXED_RING_BUFFER_H_