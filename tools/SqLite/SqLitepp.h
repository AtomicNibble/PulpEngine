#pragma once


struct sqlite3;

X_NAMESPACE_BEGIN(sql)

class DLL_EXPORT SqlLiteDb
{
	X_NO_COPY(SqlLiteDb);
	X_NO_ASSIGN(SqlLiteDb);

public:
	SqlLiteDb();
	~SqlLiteDb();

	bool connect(const char* pDb);
	bool disconnect(void);


	int errorCode(void) const;
	char const* errorMsg(void) const;

	int execute(char const* sql);
	int executef(char const* sql, ...);

private:
	sqlite3* db_;
};


X_NAMESPACE_END