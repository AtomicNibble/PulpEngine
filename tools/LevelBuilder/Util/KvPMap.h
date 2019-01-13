#pragma once

#include <Containers\HashMap.h>

X_NAMESPACE_BEGIN(level)

class KeyPair : public core::HashMap<core::string, core::string>
{
    typedef core::HashMap<core::string, core::string> BaseT;

public:
    typedef iterator PairIt;
    typedef const_iterator PairConstIt;

public:
    KeyPair(core::MemoryArenaBase* arena);
    KeyPair(const KeyPair& oth) = default;
    KeyPair(KeyPair&& oth) = default;

    KeyPair& operator=(const KeyPair& oth) = default;
    KeyPair& operator=(KeyPair&& oth) = default;

    bool GetString(const char* key, core::string_view defaultString, core::string_view& out) const;
    bool GetString(const char* key, core::string_view defaultString, core::string& out) const;
    bool GetVector(const char* key, core::string_view defaultString, Vec3f& out) const;
};

X_NAMESPACE_END