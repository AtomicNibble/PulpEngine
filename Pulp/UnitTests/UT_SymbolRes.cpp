#include "stdafx.h"

#include <Debugging\SymbolResolution.h>
#include <Debugging\CallStack.h>

X_USING_NAMESPACE;

using namespace core;

namespace
{
    int32_t line = -1;

    X_NO_INLINE CallStack Foo1(void)
    {
        line = __LINE__ + 1;
        return CallStack(0);
    }
} // namespace

TEST(Debug, SymbolRes)
{
#if X_ENABLE_SYMBOL_RESOLUTION && X_SUPER == 0
    const CallStack& stack = Foo1();
    const SymbolInfo info = symbolResolution::ResolveSymbolsForAddress(stack.GetFrame(0));

    EXPECT_EQ(line, info.GetLine());

#endif
}