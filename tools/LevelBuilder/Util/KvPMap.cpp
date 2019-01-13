#include "stdafx.h"
#include "KvPMap.h"

X_NAMESPACE_BEGIN(level)

using namespace core::string_view_literals;

KeyPair::KeyPair(core::MemoryArenaBase* arena) :
    BaseT(arena, 4)
{
}

bool KeyPair::GetString(const char* key, core::string_view defaultString, core::string_view& out) const
{
    // TODO: perf -take string_view
    if (PairConstIt it = find(core::string(key)); it != end()) {
        out = core::string_view(it->second);
        return true;
    }

    out = defaultString;
    return false;
}

bool KeyPair::GetString(const char* key, core::string_view defaultString, core::string& out) const
{
    // TODO: perf - take string_view
    if (PairConstIt it = find(core::string(key)); it != end()) {
        out = it->second;
        return true;
    }

    out = core::string(defaultString.data(), defaultString.length());
    return false;
}

bool KeyPair::GetVector(const char* key, core::string_view defaultString, Vec3f& out) const
{
    out = Vec3f::zero();

    if (defaultString.empty()) {
        defaultString = "0 0 0"_sv;
    }

    core::string_view s;
    bool found = GetString(key, defaultString, s);

    // TODO: something less shitty?
    core::StackString<128> tmp(s.begin(), s.end());
    sscanf(tmp.c_str(), "%f %f %f", &out.x, &out.y, &out.z);
    return found;
}

X_NAMESPACE_END