#pragma once

#include <Containers\HashMap.h>


X_NAMESPACE_BEGIN(lvl)

class KeyPair : public core::HashMap<core::string, core::string>
{
	typedef core::HashMap<core::string, core::string> BaseT;

public:
	typedef iterator PairIt;
	typedef const_iterator PairConstIt;

public:
	KeyPair(core::MemoryArenaBase* arena);

	bool GetString(const char* key, const char* defaultString, const char **out) const;
	bool GetString(const char* key, const char* defaultString, core::string& out) const;
	bool GetVector(const char* key, const char* defaultString, Vec3f& out) const;

};

X_NAMESPACE_END