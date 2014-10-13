#pragma once


#include <map>


class KeyPair : public std::map<core::string, core::string>
{
public:
	typedef iterator PairIt;
	typedef const_iterator PairConstIt;

	bool GetString(const char* key, const char* defaultString, const char **out) const {
		PairConstIt it = find(key);
		if (it != end()) {
			*out = it->second;
			return true;
		}
		*out = defaultString;
		return false;
	}


	bool GetString(const char* key, const char* defaultString, core::string& out) const {
		PairConstIt it = find(key);
		if (it != end()) {
			out = it->second;
			return true;
		}
		out = defaultString;
		return false;
	}



	Vec3f GetVector(const char* key, const char* defaultString = NULL) const
	{
		Vec3f out;
		GetVector(key, defaultString, out);
		return out;
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