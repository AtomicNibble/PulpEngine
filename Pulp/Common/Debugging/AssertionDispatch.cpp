#include "xLib.h"

#include "AssertionHandler.h"

X_NAMESPACE_BEGIN(core)

namespace assertionDispatch
{
    namespace
    {
        AssertionHandler* g_listHead = NULL;
        AssertionHandler* g_listTail = NULL;

    }; // namespace

    /// Adds an assertion handler to the intrusive linked-list.
    void AddAssertionHandler(AssertionHandler* handler)
    {
        if (g_listHead) {
            handler->SetPrevious(g_listTail);
            g_listTail->SetNext(handler);
            g_listTail = handler;
        }
        else {
            g_listHead = handler;
            g_listTail = handler;
        }
    }

    /// Removes an assertion handler from the intrusive linked-list.
    void RemoveAssertionHandler(AssertionHandler* handler)
    {
        AssertionHandler* prev = handler->GetPrevious();
        AssertionHandler* next = handler->GetNext();

        if (prev)
            prev->SetNext(next);
        else
            g_listHead = next;

        if (next)
            next->SetPrevious(prev);
        else
            g_listTail = prev;
    }

    /// Forwards an Assert call to all registered handlers.
    void Assert(const SourceInfo& sourceInfo)
    {
        for (AssertionHandler* handler = g_listHead; handler; handler = handler->GetNext()) {
            handler->Assert(sourceInfo);
        }
    }

    /// Forwards an Assert::Variable() call to all registered handlers.
    void AssertVariable(const SourceInfo& sourceInfo)
    {
        for (AssertionHandler* handler = g_listHead; handler; handler = handler->GetNext()) {
            handler->AssertVariable(sourceInfo);
        }
    }
} // namespace assertionDispatch

X_NAMESPACE_END