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
	KeyPair(core::MemoryArenaBase* arena) :
		BaseT(arena, 4)
	{}

	bool GetString(const char* key, const char* defaultString, const char **out) const {
		PairConstIt it = find(X_CONST_STRING(key));
		if (it != end()) {
			*out = it->second;
			return true;
		}
		*out = defaultString;
		return false;
	}


	bool GetString(const char* key, const char* defaultString, core::string& out) const {
		PairConstIt it = find(X_CONST_STRING(key));
		if (it != end()) {
			out = it->second;
			return true;
		}
		out = defaultString;
		return false;
	}


	bool GetVector(const char* key, const char* defaultString, Vec3f& out) const
	{
		bool		found;
		const char	*s;

		if (!defaultString) {
			defaultString = "0 0 0";
		}

		found = GetString(key, defaultString, &s);
		out = Vec3f::zero();
		sscanf(s, "%f %f %f", &out.x, &out.y, &out.z);
		return found;
	}

};

X_NAMESPACE_END