#pragma once

#ifndef X_SIMPLEBOUNDSCHECKING_H_
#define X_SIMPLEBOUNDSCHECKING_H_

#if X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

X_NAMESPACE_BEGIN(core)

class SimpleBoundsChecking
{
public:
    static const char* const TYPE_NAME;
    static const size_t SIZE_FRONT = 4;
    static const size_t SIZE_BACK = 4;

    void GuardFront(void* memory);
    void GuardBack(void* memory);
    void CheckFront(const void* memory);
    void CheckBack(const void* memory);

private:
    typedef unsigned char Token;

    static const Token TOKEN_FRONT = 0xAA;
    static const Token TOKEN_BACK = 0xAA;
};

X_NAMESPACE_END

#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

#endif // !X_SIMPLEBOUNDSCHECKING_H_
