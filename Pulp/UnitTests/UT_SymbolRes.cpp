#include "stdafx.h"

#include <Debugging\SymbolResolution.h>
#include <Debugging\CallStack.h>

X_USING_NAMESPACE;

using namespace core;

namespace
{
    X_NO_INLINE CallStack Foo1(void)
    {
        return CallStack(0);
    }
} // namespace

TEST(Debug, SymbolRes)
{
#if X_ENABLE_SYMBOL_RESOLUTION

    const CallStack& stack = Foo1();
    const SymbolInfo info = symbolResolution::ResolveSymbolsForAddress(stack.GetFrame(0));

    EXPECT_EQ(18, info.GetLine());

#endif
}