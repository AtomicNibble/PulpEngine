#pragma once


class asIScriptEngine;
class asIScriptContext;
struct asSMessageInfo;

X_NAMESPACE_BEGIN(assman)

class AssetProps;

class AssetPropsScript
{
	typedef std::array<std::string, assetDb::AssetType::ENUM_COUNT> StrictCacheArr;

	static const char* ASSET_PROPS_SCRIPT_EXT;
	static const char* SCRIPT_ENTRY;

public:
	AssetPropsScript();
	~AssetPropsScript();

	bool init(void);
	void shutdown(void);

	
	bool runScriptForProps(AssetProps& props, assetDb::AssetType::Enum type);

	void clearCache(void);
	void clearCache(assetDb::AssetType::Enum type);

private:
	bool cacheValid(assetDb::AssetType::Enum type) const;
	bool ensureCache(assetDb::AssetType::Enum type, bool reload = false);
	bool loadScript(assetDb::AssetType::Enum type, std::string& out);
	bool loadScript(const core::Path<char>& path, std::string& out);
	bool processScript(AssetProps& props, const std::string& script);

private:
	static void messageCallback(const asSMessageInfo *msg, void *param);
	static void printExceptionInfo(asIScriptContext *ctx);

private:
	asIScriptEngine* pEngine_;
	StrictCacheArr cache_;
};



X_NAMESPACE_END