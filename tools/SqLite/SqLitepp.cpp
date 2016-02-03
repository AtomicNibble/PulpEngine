#include "stdafx.h"
#include "SqLitepp.h"

#include <../../3rdparty/source/sqlite-3.10.1/sqlite3.h>
// #include <../../3rdparty/source/sqlite-3.10.1/sqlite3ext.h>

#include <ModuleExports.h>

X_NAMESPACE_BEGIN(sql)


SqlLiteDb::SqlLiteDb()
{

}

SqlLiteDb::~SqlLiteDb()
{

}

bool SqlLiteDb::connect(const char* pDb)
{
	X_ASSERT_NOT_NULL(pDb);

	int ret;

	if (!disconnect()) {
		X_ERROR("SqlLiteDb", "Failed to disconeect beofre conencting to new db: \"%s\"", pDb);
		return false;
	}

	if (SQLITE_OK != (ret = sqlite3_initialize()))
	{
		X_ERROR("SqlLiteDb", "Failed to initialize library: %d", ret);
		return false;
	}

	// open connection to a DB
	if (SQLITE_OK != (ret = sqlite3_open_v2(pDb, &db_,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr)))
	{
		X_ERROR("SqlLiteDb", "Failed to open conn: %d", ret);
		return false;
	}

	return false;
}

bool SqlLiteDb::disconnect(void)
{
	if (db_) 
	{
		int ret, ret2;
		if (SQLITE_OK != (ret = sqlite3_close(db_))) {
			X_ERROR("SqlLiteDb", "Failed to close db");
		}

		if(SQLITE_OK != (ret2 = sqlite3_shutdown())) {
			X_ERROR("SqlLiteDb", "Failed to shutdown sqLite");
		}

		return ret == SQLITE_OK && ret2 == SQLITE_OK;
	}
	return true;
}

int SqlLiteDb::errorCode(void) const
{
	return sqlite3_errcode(db_);
}

char const* SqlLiteDb::errorMsg(void) const
{
	return sqlite3_errmsg(db_);
}


int SqlLiteDb::execute(char const* sql)
{	
	return sqlite3_exec(db_, sql, 0, 0, 0);
}

int SqlLiteDb::executef(char const* sql, ...)
{
	va_list ap;
	va_start(ap, sql);
	std::shared_ptr<char> msql(sqlite3_vmprintf(sql, ap), sqlite3_free);
	va_end(ap);

	return execute(msql.get());
}

X_NAMESPACE_END