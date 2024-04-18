#include "stdafx.h"
#include "UT_AssertChecker.h"

X_NAMESPACE_BEGIN(core)

UtAssetCheckerHandler::UtAssetCheckerHandler(void) :
    m_expectAssertion(false),
    m_hadExpectedAssertion(false),
    m_hadUnexpectedAssertion(false)
{
}

UtAssetCheckerHandler::~UtAssetCheckerHandler(void)
{
}

void UtAssetCheckerHandler::ExpectAssertion(bool expect)
{
    m_expectAssertion = expect;
    m_hadExpectedAssertion = false;
    m_hadUnexpectedAssertion = false;
}

bool UtAssetCheckerHandler::HadCorrectAssertions(void) const
{
#if X_ENABLE_ASSERTIONS
    return m_hadExpectedAssertion;
#else // X_ENABLE_ASSERTIONS
    X_WARNING("UTAssert", "Assertions are disabled, unable to check if working.");
    return true;
#endif // X_ENABLE_ASSERTIONS
}

void UtAssetCheckerHandler::OnAssert(const SourceInfo& sourceInfo)
{
    bool expected = m_expectAssertion;

    m_hadExpectedAssertion = expected;
    m_hadUnexpectedAssertion = !expected;

    if (expected) {
        X_LOG0("UTAssert", "An expected asset was triggered");
    }
    else {
        X_LOG0("UTAssert", "An unexpected asset was triggered");
    }
}

void UtAssetCheckerHandler::OnAssertVariable(const SourceInfo& sourceInfo)
{
    X_UNUSED(sourceInfo);
    // o hi.
}

X_NAMESPACE_END
