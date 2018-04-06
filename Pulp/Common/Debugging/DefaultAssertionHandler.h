#pragma once

#ifndef X_DEFAULTASSERTIONHANDLER_H
#define X_DEFAULTASSERTIONHANDLER_H

#include "ICore.h"

X_NAMESPACE_BEGIN(core)

struct SourceInfo;

/// \ingroup Debugging
/// \class DefaultAssertionHandler
/// \brief Default implementation of an assertion handler.
/// \details This class implements the AssertionHandler interface, but leaves all functions empty. It should always
/// be added to the assertionDispatch in order to provide a single point of user-customization, and can serve as a
/// starting point for implementing other handlers.
/// \sa X_ASSERT Assert AssertionHandler
class DefaultAssertionHandler : public IAssertHandler
{
public:
    // Default constructor.
    DefaultAssertionHandler(void);

    // Destructor.
    virtual ~DefaultAssertionHandler(void);

private:
    /// Empty implementation.
    virtual void DoAssert(const SourceInfo& sourceInfo) X_OVERRIDE;

    /// Empty implementation.
    virtual void DoAssertVariable(const SourceInfo& sourceInfo) X_OVERRIDE;
};

X_NAMESPACE_END

#endif
