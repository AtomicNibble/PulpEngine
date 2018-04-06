#pragma once

#ifndef X_CRITICALSECTION_H_
#define X_CRITICALSECTION_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Threading
/// \class CriticalSection
/// \brief A class representing a critical section.
/// \details This class wraps OS-specific critical sections which are used to protect sections of code from being
/// accessed by more than one thread at the same time.
///
/// An example is given below:
/// \code
///   core::CriticalSection cs;
///
///   // assume this function is called from several different threads
///   void AnyClass::Function(void)
///   {
///     ...
///
///     {
///       // enter the critical section, modify the member, and leave the critical section again
///       core::CriticalSection::ScopedLock lock(cs);
///
///       // the code in this scope can only be accessed from one thread at a time.
///       // shared data can safely be accessed and modified here.
///       m_someSharedData = ...;
///     }
///
///     ...
///   }
/// \endcode
/// \warning Manually calling CriticalSection::Enter() and CriticalSection::Leave() should only be used in rare
/// circumstances, such as when entering the critical section in one function, but leaving it in a different function.
/// In all other occasions, one should always use a ScopedLock. This eliminates the risk of those dead-locks that are
/// caused by forgetting to leave a critical section in code having multiple exit paths.
/// \sa Thread ConditionVariable Semaphore Spinlock
class CriticalSection
{
public:
    /// Initializes the critical section.
    CriticalSection(void);

    /// \brief Initializes the critical section with a certain spin count.
    /// \remark Entering the critical section will first try spinning the given number of times before finally acquiring
    /// the critical section if spinning was unsuccessful.
    explicit CriticalSection(uint32_t spinCount);

    /// Releases OS resources of the critical section.
    ~CriticalSection(void);

    /// Enters the critical section.
    void Enter(void);

    /// Tries to enter the critical section, and returns whether the operation was successful.
    bool TryEnter(void);

    /// Leaves the critical section.
    void Leave(void);

    /// set number of sinds before acquirung critical section.
    void SetSpinCount(uint32_t count);

    /// \brief Returns a pointer to the native object.
    /// \remark For internal use only.
    inline CRITICAL_SECTION* GetNativeObject(void);

    /// \class ScopedLock
    /// \brief A RAII-style scoped lock.
    /// \details If used as an automatic variable on the stack, the ScopedLock will Enter() the critical section in
    /// the constructor, and Leave() it in the destructor. This eliminates the risk of forgetting to leave a critical
    /// section in code having multiple exit paths.
    ///
    /// See http://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization for an explanation of the RAII idiom.
    class ScopedLock
    {
    public:
        /// Enters the given critical section.
        inline explicit ScopedLock(CriticalSection& criticalSection);

        /// Leaves the critical section.
        inline ~ScopedLock(void);

    private:
        X_NO_COPY(ScopedLock);
        X_NO_ASSIGN(ScopedLock);

        CriticalSection& cs_;
    };

private:
    CRITICAL_SECTION cs_;
};

#include "CriticalSection.inl"

X_NAMESPACE_END

#endif // X_CRITICALSECTION_H_
