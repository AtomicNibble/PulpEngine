#include "stdafx.h"
#include "KvPMap.h"

X_NAMESPACE_BEGIN(level)

KeyPair::KeyPair(core::MemoryArenaBase* arena) :
    BaseT(arena, 4)
{
}

bool KeyPair::GetString(const char* key, const char* defaultString, const char** out) const
{
    PairConstIt it = find(core::string(key));
    if (it != end()) {
        *out = it->second;
        return true;
    }
    *out = defaultString;
    return false;
}

bool KeyPair::GetString(const char* key, const char* defaultString, core::string& out) const
{
    PairConstIt it = find(core::string(key));
    if (it != end()) {
        out = it->second;
        return true;
    }
    out = defaultString;
    return false;
}

bool KeyPair::GetVector(const char* key, const char* defaultString, Vec3f& out) const
{
    bool found;
    const char* s;

    if (!defaultString) {
        defaultString = "0 0 0";
    }

    found = GetString(key, defaultString, &s);
    out = Vec3f::zero();
    sscanf(s, "%f %f %f", &out.x, &out.y, &out.z);
    return found;
}

X_NAMESPACE_END