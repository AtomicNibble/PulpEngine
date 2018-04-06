#pragma once

#ifndef X_DEBUGGING_ASSERT_HANDLER_H_
#define X_DEBUGGING_ASSERT_HANDLER_H_

X_NAMESPACE_BEGIN(core)

struct SourceInfo;

class AssertionHandler
{
public:
    /// Default constructor, initializing the linked-list pointers.
    AssertionHandler(void);

    /// Destructor.
    virtual ~AssertionHandler(void);

    /// Called whenever an assert fires.
    void Assert(const SourceInfo& sourceInfo);

    /// Called whenever an Assert::Variable() call is made.
    void AssertVariable(const SourceInfo& sourceInfo);

    /// \brief Sets the previous handler in the intrusive linked-list.
    /// \remark Used internally for maintaining the intrusive linked-list.
    inline void SetPrevious(AssertionHandler* handler);

    /// \brief Sets the next handler in the intrusive linked-list.
    /// \remark Used internally for maintaining the intrusive linked-list.
    inline void SetNext(AssertionHandler* handler);

    /// \brief Returns the previous handler in the intrusive linked-list.
    /// \remark Used internally for maintaining the intrusive linked-list.
    inline AssertionHandler* GetPrevious(void) const;

    /// \brief Returns the next handler in the intrusive linked-list.
    /// \remark Used internally for maintaining the intrusive linked-list.
    inline AssertionHandler* GetNext(void) const;

private:
    /// Customization point for derived classes.
    virtual void DoAssert(const SourceInfo& sourceInfo) ME_ABSTRACT;

    /// Customization point for derived classes.
    virtual void DoAssertVariable(const SourceInfo& sourceInfo) ME_ABSTRACT;

    AssertionHandler* m_previous;
    AssertionHandler* m_next;
};

#include "AssertionHandler.inl"

X_NAMESPACE_END

#endif // X_DEBUGGING_ASSERT_HANDLER_H_