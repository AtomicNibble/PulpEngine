


X_INLINE Spinlock::Spinlock() :
	locked_(0)
{
}

X_INLINE void Spinlock::Enter(void)
{
	for (;;) {
		if (atomic::CompareExchange(&locked_, 1, 0) == 0)
			break;
	}
}

X_INLINE bool Spinlock::TryEnter(void)
{
	return atomic::CompareExchange(&locked_, 1, 0) == 0;
}

X_INLINE bool Spinlock::TryEnter(unsigned int tries)
{
	while (tries > 0) {
		if (atomic::CompareExchange(&locked_, 1, 0) == 0)
			return true;
		tries++;
	}
	return false;
}

X_INLINE void Spinlock::Leave(void)
{
	atomic::Exchange(&locked_, 0);
}

// -------------------------------------------

X_INLINE Spinlock::ScopedLock::ScopedLock(Spinlock& spinlock) :
	spinlock_(spinlock)
{
	spinlock.Enter();
}

X_INLINE Spinlock::ScopedLock::~ScopedLock(void)
{
	spinlock_.Leave();
}
