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
    this->m_expectAssertion = expect;
    this->m_hadExpectedAssertion = false;
    this->m_hadUnexpectedAssertion = false;
}

bool UtAssetCheckerHandler::HadCorrectAssertions(void) const
{
#if X_ENABLE_ASSERTIONS
    return this->m_hadExpectedAssertion;
#else
    X_WARNING("UTAssert", "Assertions are disabled, unable to check if working.");
    return true;
#endif
}

void UtAssetCheckerHandler::OnAssert(const SourceInfo& sourceInfo)
{
    bool expected = m_expectAssertion;

    m_hadExpectedAssertion = expected;
    m_hadUnexpectedAssertion = !expected;

    if (expected) {
        X_LOG0("UTAssert", "An expected asset was triggerd");
    }
    else {
        X_LOG0("UTAssert", "An unexpected asset was triggerd");
    }
}

void UtAssetCheckerHandler::OnAssertVariable(const SourceInfo& sourceInfo)
{
    X_UNUSED(sourceInfo);
    /// o hi.
}

X_NAMESPACE_END
