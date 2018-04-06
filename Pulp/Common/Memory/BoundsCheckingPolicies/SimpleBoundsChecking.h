#pragma once

#ifndef X_SIMPLEBOUNDSCHECKING_H_
#define X_SIMPLEBOUNDSCHECKING_H_

#if X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements a bounds checking policy for memory arenas.
/// \details This class implements the concepts of a bounds checking policy as expected by the MemoryArena class. It
/// adds 4 bytes to the front and back of an allocation, and marks these regions with special tokens. Whenever an
/// allocation is freed, the bounds checker scans these tokenized regions. If they contain a bit pattern different
/// from the original token, then a memory stomp has been detected.
/// \sa NoBoundsChecking
class SimpleBoundsChecking
{
public:
    /// A human-readable string literal containing the policy's type.
    static const char* const TYPE_NAME;

    /// Defines the number of guard bytes at the front of an allocation.
    static const size_t SIZE_FRONT = 4;

    /// Defines the number of guard bytes at the back of an allocation.
    static const size_t SIZE_BACK = 4;

    /// Guards \c SIZE_FRONT bytes at the given memory address with \c TOKEN_FRONT.
    void GuardFront(void* memory);

    /// Guards \c SIZE_BACK bytes at the given memory address with \c TOKEN_BACK.
    void GuardBack(void* memory);

    /// Checks whether \c SIZE_FRONT bytes at the given memory address contain \c TOKEN_FRONT.
    void CheckFront(const void* memory);

    /// Checks whether \c SIZE_BACK bytes at the given memory address contain \c TOKEN_BACK.
    void CheckBack(const void* memory);

private:
    /// A type representing guard tokens.
    typedef unsigned char Token;

    /// Defines the token which is used to mark the front guard of a memory region.
    static const Token TOKEN_FRONT = 0xAA;

    /// Defines the token which is used to mark the back guard of a memory region.
    static const Token TOKEN_BACK = 0xAA;
};

X_NAMESPACE_END

#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

#endif // !X_SIMPLEBOUNDSCHECKING_H_
