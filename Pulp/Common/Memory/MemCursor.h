#pragma once

#ifndef X_MEMORY_CURSOR_H_
#define X_MEMORY_CURSOR_H_

X_NAMESPACE_BEGIN(core)

class MemCursor
{
public:
    MemCursor(void* const pData, size_t size) :
        pPointer_(reinterpret_cast<char*>(pData)),
        pStart_(reinterpret_cast<char*>(pData)),
        size_(size)
    {
    }

    template<typename Type>
    Type* postSeekPtr(size_t num)
    {
        Type* cur = getPtr<Type>();
        seek<Type>(num);
        return cur;
    }

    template<typename Type>
    const Type get(void) const
    {
        return *getPtr<Type>();
    }

    template<typename Type>
    const Type* getPtr(void) const
    {
        return reinterpret_cast<Type*>(pPointer_);
    }

    template<typename Type>
    Type* getPtr(void)
    {
        return reinterpret_cast<Type*>(pPointer_);
    }

    template<typename Type>
    const Type getSeek(void)
    {
        return *getSeekPtr<Type>();
    }

    template<typename Type>
    const Type* getSeekPtr(void) const
    {
        Type* val = getPtr<Type>();
        seek<Type>(1);
        return val;
    }

    template<typename Type>
    Type* getSeekPtr(void)
    {
        Type* val = getPtr<Type>();
        seek<Type>(1);
        return val;
    }

    void operator++(void)
    {
        seek<char>(1);
    }

    bool isEof(void) const
    {
        size_t offset = union_cast<size_t>(pPointer_ - pStart_);
        return offset >= size_;
    }

    size_t numBytesRemaning(void) const
    {
        size_t offset = union_cast<size_t>(pPointer_ - pStart_);
        return size_ - offset;
    }

    void seekBytes(size_t num)
    {
        seek<char>(num);
    }

    template<typename Type>
    void seek(size_t num)
    {
        union
        {
            Type* as_type;
            char* as_self;
        };

        as_self = pPointer_;
        as_type += num;
        pPointer_ = as_self;
    }

    char* begin(void) const
    {
        return pPointer_;
    }

    char* end(void) const
    {
        return pStart_ + size_;
    }

private:
    char* pPointer_;
    char* pStart_;
    size_t size_;
};

X_NAMESPACE_END

#endif // X_MEMORY_CURSOR_H_