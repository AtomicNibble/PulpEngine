#include "xLib.h"

#include "AssertionHandler.h"

X_NAMESPACE_BEGIN(core)

AssertionHandler::AssertionHandler(void) :
    m_previous(NULL),
    m_next(NULL)
{
}

/// Destructor.
AssertionHandler::~AssertionHandler(void)
{
}

/// Called whenever an assert fires.
void AssertionHandler::Assert(const SourceInfo& sourceInfo)
{
    this->DoAssert(sourceInfo);
}

/// Called whenever an Assert::Variable() call is made.
void AssertionHandler::AssertVariable(const SourceInfo& sourceInfo)
{
    this->DoAssertVariable(sourceInfo);
}

X_NAMESPACE_END
