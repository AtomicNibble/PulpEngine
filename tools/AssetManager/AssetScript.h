#pragma once


class asIScriptEngine;
class asIScriptContext;
struct asSMessageInfo;

X_NAMESPACE_BEGIN(assman)

class AssetProps;

class AssetPropsScript
{
	typedef std::array<AssetProps*, assetDb::AssetType::ENUM_COUNT> PropsCaheArr;

	static const char* ASSET_PROPS_SCRIPT_EXT;
	static const char* SCRIPT_ENTRY;

public:
	AssetPropsScript();
	~AssetPropsScript();

	bool init(void);
	void shutdown(void);

	
	bool getAssetPropsForType(assetDb::AssetType::Enum type, AssetProps& props, bool reload = false);

private:
	bool loadFromCache(AssetProps& props, assetDb::AssetType::Enum type);
	void setCache(AssetProps& props, assetDb::AssetType::Enum type);
	void clearCache(void);

	bool processScriptForType(AssetProps& props, assetDb::AssetType::Enum type);
	bool processScript(AssetProps& props, const char* pFileName);

private:
	static void messageCallback(const asSMessageInfo *msg, void *param);
	static void printExceptionInfo(asIScriptContext *ctx);

private:
	asIScriptEngine* pEngine_;
	PropsCaheArr cache_;
};



X_NAMESPACE_END