#include "stdafx.h"
#include "Script.h"

X_NAMESPACE_BEGIN(script)

using namespace lua;

Script::Script(core::MemoryArenaBase* arena, core::string& name) :
	core::AssetBase(name, assetDb::AssetType::SCRIPT),
	dataSize_(0),
	chunk_(lua::Ref::Nil),
	pPendingInclude_(nullptr),
	lastResult_(lua::CallResult::None),
	hash_(0)
{
	X_UNUSED(arena);
}

bool Script::processData(core::UniquePointer<char[]> data, uint32_t dataSize)
{
	auto hash = core::Hash::xxHash64::getHash(data.ptr(), dataSize);

	data_ = std::move(data);
	dataSize_ = dataSize;
	hash_ = hash;
	return true;
}



lua::RefId Script::getChunk(lua_State* L)
{
	X_LUA_CHECK_STACK(L);

	if (chunk_ == lua::Ref::Nil)
	{
		auto result = state::load(L, begin(), end(), name_.c_str());
		if (result != LoadResult::Ok) {
			const char* pErr = stack::as_string(L);
			X_ERROR("Script", "Failed to load \"%s.%s\" res: %s err: \"%s\"",
				name_.c_str(), SCRIPT_FILE_EXTENSION, LoadResult::ToString(result), pErr);
			stack::pop(L);
			return lua::Ref::Nil;
		}

		chunk_ = stack::pop_to_ref(L);
	}

	return chunk_;
}

X_NAMESPACE_END