#pragma once


#include <String/StringView.h>
#include <String/StringHash.h>

X_NAMESPACE_BEGIN(locale)


// Potentially at somepoint I might want auto localisation.
// Which would mean loading the current table then parsing assets for strings and auto replacing them with keys.
// then adding current value to table.
// 
static const size_t MAX_STRINGS = 512;

struct ILocalisation
{
    using string_view = core::string_view;
    using Key = core::StrHash;

    virtual ~ILocalisation() {}

    virtual string_view getString(Key strHash) const X_ABSTRACT;

};


X_NAMESPACE_END