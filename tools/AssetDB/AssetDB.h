#pragma once



X_NAMESPACE_BEGIN(assetDb)


class AssetDB
{
	static const char* DB_NAME;

public:
	AssetDB();
	~AssetDB();

	bool OpenDB(void);
	void CloseDB(void);

private:
	bool IsDbOpen(void) const;
	
	bool CreateTables(void);

private:
	sqlite3* db_;
};

X_NAMESPACE_END