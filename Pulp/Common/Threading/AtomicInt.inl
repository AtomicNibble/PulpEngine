

X_INLINE AtomicInt::AtomicInt(int32_t value)
: value_(value)
{
}

X_INLINE int32_t AtomicInt::operator=(int32_t value) volatile
{
	return atomic::Exchange(&value_, value);
}

X_INLINE int32_t AtomicInt::operator=(int32_t value)
{
	return atomic::Exchange(&value_, value);
}

X_INLINE int32_t AtomicInt::operator+=(int32_t value) volatile
{
	return atomic::Add(&value_, value);
}

X_INLINE int32_t AtomicInt::operator+=(int32_t value)
{
	return atomic::Add(&value_, value);
}

X_INLINE int32_t AtomicInt::operator++(void)
{
	return atomic::Increment(&value_);
}

X_INLINE int32_t AtomicInt::operator++(void) volatile
{
	return atomic::Increment(&value_);
}


X_INLINE int32_t AtomicInt::operator--(void) volatile
{
	return atomic::Decrement(&value_);
}

X_INLINE int32_t AtomicInt::operator--(void)
{
	return atomic::Decrement(&value_);
}

X_INLINE AtomicInt::operator int32_t(void) const volatile
{
	return value_;
}

X_INLINE AtomicInt::operator int32_t(void) const
{
	return value_;
}

