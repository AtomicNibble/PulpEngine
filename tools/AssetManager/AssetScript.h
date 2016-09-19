#pragma once


class asIScriptEngine;
class asIScriptContext;
struct asSMessageInfo;

X_NAMESPACE_BEGIN(assman)

class AssetProps;

class AssetPropsScript
{
	class ByteCodeStream : public asIBinaryStream
	{
	public:
		ByteCodeStream();
		~ByteCodeStream() = default;

		bool isNotEmpty(void) const;
		void clear(void);
		void resetRead(void);

		void Write(const void *ptr, asUINT size) X_OVERRIDE;
		void Read(void *ptr, asUINT size) X_OVERRIDE;

	protected:
		size_t readPos_;
		std::vector<uint8_t> data_;
	};

	struct Scriptcache
	{
		void clear(bool byteCodeOnly);

		std::string text;
		ByteCodeStream byteCode;
	};

	typedef std::array<Scriptcache, assetDb::AssetType::ENUM_COUNT> ScriptCacheArr;

	static const char* ASSET_PROPS_SCRIPT_EXT;
	static const char* SCRIPT_ENTRY;

public:
	AssetPropsScript();
	~AssetPropsScript();

	bool init(void);
	void shutdown(void);

	
	bool runScriptForProps(AssetProps& props, assetDb::AssetType::Enum type);

	void clearCache(bool byteCodeOnly = false);
	void clearCache(assetDb::AssetType::Enum type, bool byteCodeOnly = false);

private:
	bool sourceCacheValid(assetDb::AssetType::Enum type) const;
	bool ensureSourceCache(assetDb::AssetType::Enum type, bool reload = false);

	bool loadScript(assetDb::AssetType::Enum type, std::string& out);
	bool loadScript(const core::Path<char>& path, std::string& out);
	bool processScript(AssetProps& props, Scriptcache& cache);
	bool processScript(AssetProps& props, const std::string& script, ByteCodeStream* pCacheOut);
	bool processScript(AssetProps& props, ByteCodeStream& cache);
	bool execScript(AssetProps& props, asIScriptModule* pMod);


private:
	static void messageCallback(const asSMessageInfo *msg, void *param);
	static void printExceptionInfo(asIScriptContext *ctx);

private:
	asIScriptEngine* pEngine_;
	ScriptCacheArr cache_;
};



X_NAMESPACE_END