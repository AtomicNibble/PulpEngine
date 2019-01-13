#include "EngineCommon.h"
#include "StringView.h"

#include "StrRef.h"

X_NAMESPACE_BEGIN(core)


string_view::string_view(const StringRef<value_type>& str) :
    pBegin_(str.data()),
    size_(str.length())
{
}


X_NAMESPACE_END