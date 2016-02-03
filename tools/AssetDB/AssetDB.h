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

	bool CreateTables(void);

private:
	sql::SqlLiteDb db_;
};

X_NAMESPACE_END