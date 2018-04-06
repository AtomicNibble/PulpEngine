#pragma once

#ifndef X_UT_ASSERT_CHECKER_H_
#define X_UT_ASSERT_CHECKER_H_

X_NAMESPACE_BEGIN(core)

class UtAssetCheckerHandler : public IAssertHandler
{
public:
    UtAssetCheckerHandler(void);
    virtual ~UtAssetCheckerHandler(void);

    void ExpectAssertion(bool expect);
    bool HadCorrectAssertions(void) const;

private:
    virtual void OnAssert(const SourceInfo& sourceInfo) X_OVERRIDE;
    virtual void OnAssertVariable(const SourceInfo& sourceInfo) X_OVERRIDE;

    bool m_expectAssertion;
    bool m_hadExpectedAssertion;
    bool m_hadUnexpectedAssertion;
};

X_NAMESPACE_END

#endif // X_UT_ASSERT_CHECKER_H_