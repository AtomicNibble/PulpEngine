#pragma once


#ifndef X_MEMORY_CURSOR_H_
#define X_MEMORY_CURSOR_H_

X_NAMESPACE_BEGIN(core)

class MemCursor
{
public:
	MemCursor(void* const pData, size_t size) : 
		pPointer_(pData), 
		pStart_(pData), 
		size_(size) 
	{}

	template<typename Type>
	Type* postSeekPtr(size_t num)
	{
		Type* cur = (Type*)pPointer_;

		Seek<Type>(num);
		return cur;
	}

	template<typename Type>
	const Type get() const
	{
		return (Type)*getPtr<Type>();
	}

	template<typename Type>
	const Type* getPtr() const
	{
		return (Type*)pPointer_;
	}

	template<typename Type>
	Type* getPtr()
	{
		return (Type*)pPointer_;
	}

	template<typename Type>
	const Type getSeek()
	{
		return *getSeekPtr<Type>();
	}

	template<typename Type>
	const Type* getSeekPtr() const
	{
		Type* val = (Type*)pPointer_;
		Seek<Type>(1);
		return val;
	}

	template<typename Type>
	Type* getSeekPtr()
	{
		Type* val = (Type*)pPointer_;
		Seek<Type>(1);
		return val;
	}

	void operator ++(void) {
		Seek<BYTE>(1);
	}

	bool isEof() const {
		size_t offset = ((BYTE*)pPointer_ - (BYTE*)pStart_);
		return offset >= size_;
	}

	size_t numBytesRemaning(void) const {
		size_t offset = ((BYTE*)pPointer_ - (BYTE*)pStart_);
		return size_ - offset;
	}

	void SeekBytes(int32_t num) {
		Seek<BYTE>(num);
	}

	template<typename Type>
	void Seek(size_t num) {
		union {
			Type* as_type;
			void* as_self;
		};

		as_self = pPointer_;
		as_type += num;
		pPointer_ = as_self;
	}

private:
	void* pPointer_;
	void* pStart_;
	size_t size_;
};

X_NAMESPACE_END

#endif // X_MEMORY_CURSOR_H_