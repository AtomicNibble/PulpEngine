#pragma once

#include <Hashing\xxHash.h>

X_NAMESPACE_BEGIN(script)

class XScriptTable;
class XScriptBinds;

class Script : public core::AssetBase
    , public IScript
{
    X_NO_COPY(Script);
    X_NO_ASSIGN(Script);

public:
    typedef core::Hash::xxHash64::HashVal HashVal;

public:
    Script(core::MemoryArenaBase* arena, core::string& name);

    bool processData(core::UniquePointer<char[]> data, uint32_t dataSize);

    bool loadScript(lua_State* L);

    const char* begin(void) const;
    const char* end(void) const;

    lua::RefId getChunk(lua_State* L);

    X_INLINE void setPendingInclude(Script* pScript)
    {
        pPendingInclude_ = pScript;
    }
    X_INLINE Script* getPendingInclude(void) const
    {
        return pPendingInclude_;
    }
    X_INLINE bool hasPendingInclude(void) const
    {
        return pPendingInclude_ != nullptr;
    }

    X_INLINE lua::CallResult::Enum getLastCallResult(void) const
    {
        return lastResult_;
    }

    X_INLINE void setLastCallResult(lua::CallResult::Enum res)
    {
        lastResult_ = res;
    }

    X_INLINE HashVal getHash(void) const
    {
        return hash_;
    }

private:
    core::UniquePointer<char[]> data_;
    uint32_t dataSize_;
    lua::RefId chunk_;

    lua::CallResult::Enum lastResult_;
    Script* pPendingInclude_;

    HashVal hash_;
};

X_INLINE const char* Script::begin(void) const
{
    return data_.ptr();
}

X_INLINE const char* Script::end(void) const
{
    return data_.ptr() + dataSize_;
}

X_NAMESPACE_END