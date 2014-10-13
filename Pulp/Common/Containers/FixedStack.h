#pragma once
#ifndef X_CON_FIXEDSTACK_H_
#define X_CON_FIXEDSTACK_H_


X_NAMESPACE_BEGIN(core)


template <typename T, size_t N>
class FixedStack
{
public:
	// Constructs a stack which is capable of holding \a capacity items.
	inline FixedStack(void);

	// Pushes a new value onto the stack.
	void push(const T& value);
	// Pops a value from the stack.
	void pop(void);
	// Returns the topmost value on the stack.
	inline T& top(void);
	// Returns the topmost value on the stack.
	inline const T& top(void) const;

	// any iterms in the stack
	inline bool isEmpty(void) const;

	// clears all objects but dose not free memory.
	inline void clear(void);

	// returns the number of elemets in the stack currently
	inline size_t size() const;
	// returns the number of elements this stack can currently hold.
	inline size_t capacity() const;

	/// Defines an iterator for STL-style iteration.
	typedef T* iterator;
	typedef const T* const_iterator;

	inline iterator begin(void);
	inline const_iterator begin(void) const;
	inline iterator end(void);
	inline const_iterator end(void) const;

private:
	X_NO_COPY(FixedStack);
	X_NO_ASSIGN(FixedStack);

	T	array_[N];
	T*	current_;					
};

#include "FixedStack.inl"

X_NAMESPACE_END

#endif // !X_CON_FIXEDSTACK_H_
