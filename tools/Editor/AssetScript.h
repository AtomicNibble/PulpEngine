#pragma once

#include <QObject>

class asIScriptEngine;
class asIScriptContext;
struct asSMessageInfo;

X_NAMESPACE_DECLARE(engine,
	namespace techset
	{
		class TechSetDefs;
	} // namespace techset
);


X_NAMESPACE_BEGIN(editor)

class AssetProperties;

class AssetPropsScriptManager : public QObject
{
	Q_OBJECT


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

	static const char* ASSET_PROPS_SCRIPT_DIR;
	static const char* ASSET_PROPS_SCRIPT_EXT;
	static const char* SCRIPT_ENTRY;

public:
	AssetPropsScriptManager();
	~AssetPropsScriptManager();

	bool init(bool enableHotReload = true);
	void shutdown(void);

	// runs the script against the props instance.
	bool runScriptForProps(AssetProperties& props, assetDb::AssetType::Enum type);

	void clearCache(bool byteCodeOnly = false);
	void clearCache(assetDb::AssetType::Enum type, bool byteCodeOnly = false);

private:
	bool sourceCacheValid(assetDb::AssetType::Enum type) const;
	bool ensureSourceCache(assetDb::AssetType::Enum type, bool reload = false);

	bool loadScript(assetDb::AssetType::Enum type, std::string& out);
	bool loadScript(const core::Path<char>& path, std::string& out);
	bool processScript(AssetProperties& props, Scriptcache& cache);
	bool processScript(AssetProperties& props, const std::string& script, ByteCodeStream* pCacheOut);
	bool processScript(AssetProperties& props, ByteCodeStream& cache);
	bool execScript(AssetProperties& props, asIScriptModule* pMod);

private:
	static void messageCallback(const asSMessageInfo *msg, void *param);
	static void printExceptionInfo(asIScriptContext *ctx);

private slots:
	// misc
	void fileChanged(const QString& path);

private:
	asIScriptEngine* pEngine_;
	QFileSystemWatcher* pWatcher_;
	engine::techset::TechSetDefs* pTechDefs_;
	ScriptCacheArr cache_;
};



X_NAMESPACE_END