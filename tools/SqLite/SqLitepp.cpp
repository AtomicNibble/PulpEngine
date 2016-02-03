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


int SqlLiteDb::enableForeignKeys(bool enable)
{
	return sqlite3_db_config(db_, SQLITE_DBCONFIG_ENABLE_FKEY, enable ? 1 : 0, nullptr);
}

int SqlLiteDb::enableTriggers(bool enable)
{
	return sqlite3_db_config(db_, SQLITE_DBCONFIG_ENABLE_TRIGGER, enable ? 1 : 0, nullptr);
}

int SqlLiteDb::enableExtendedResultCodes(bool enable)
{
	return sqlite3_extended_result_codes(db_, enable ? 1 : 0);
}


SqlLiteDb::RowId SqlLiteDb::lastInsertRowid(void) const
{
	return sqlite3_last_insert_rowid(db_);

}

int SqlLiteDb::errorCode(void) const
{
	return sqlite3_errcode(db_);
}

const char* SqlLiteDb::errorMsg(void) const
{
	return sqlite3_errmsg(db_);
}


int SqlLiteDb::execute(const char* sql)
{	
	return sqlite3_exec(db_, sql, 0, 0, 0);
}

int SqlLiteDb::executef(const char* sql, ...)
{
	va_list ap;
	va_start(ap, sql);
	std::shared_ptr<char> msql(sqlite3_vmprintf(sql, ap), sqlite3_free);
	va_end(ap);

	return execute(msql.get());
}

// ----------------------------------------------------

SqlLiteStateMnt::SqlLiteStateMnt(SqlLiteDb& db, const char* pStmt) : 
	db_(db), 
	stmt_(0), 
	tail_(0)
{
	if (pStmt) {
		auto rc = prepare(pStmt);

	}
}

SqlLiteStateMnt::~SqlLiteStateMnt()
{
}

int SqlLiteStateMnt::prepare(const char* pStmt)
{
	auto rc = finish();
	if (rc != SQLITE_OK) {
		return rc;
	}

	return prepare_impl(pStmt);
}


int SqlLiteStateMnt::prepare_impl(const char* pStmt)
{
	return sqlite3_prepare(db_.db_, pStmt, std::strlen(pStmt), &stmt_, &tail_);
}

int SqlLiteStateMnt::finish(void)
{
	auto rc = SQLITE_OK;
	if (stmt_) {
		rc = finish_impl(stmt_);
		stmt_ = nullptr;
	}

	tail_ = nullptr;
	return rc;
}

int SqlLiteStateMnt::finish_impl(sqlite3_stmt* pStmt)
{
	return sqlite3_finalize(pStmt);
}

int SqlLiteStateMnt::step(void)
{
	return sqlite3_step(stmt_);
}

int SqlLiteStateMnt::reset(void)
{
	return sqlite3_reset(stmt_);
}

int SqlLiteStateMnt::bind(int idx, int value)
{
	return sqlite3_bind_int(stmt_, idx, value);
}

int SqlLiteStateMnt::bind(int idx, double value)
{
	return sqlite3_bind_double(stmt_, idx, value);
}

int SqlLiteStateMnt::bind(int idx, long long int value)
{
	return sqlite3_bind_int64(stmt_, idx, value);
}

int SqlLiteStateMnt::bind(int idx, const char* value, CopySemantic::Enum  fcopy)
{
	return sqlite3_bind_text(stmt_, idx, value, std::strlen(value), fcopy == CopySemantic::COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

int SqlLiteStateMnt::bind(int idx, void const* value, int n, CopySemantic::Enum  fcopy)
{
	return sqlite3_bind_blob(stmt_, idx, value, n, fcopy == CopySemantic::COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

int SqlLiteStateMnt::bind(int idx, std::string const& value, CopySemantic::Enum  fcopy)
{
	return sqlite3_bind_text(stmt_, idx, value.c_str(), value.size(), fcopy == CopySemantic::COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

int SqlLiteStateMnt::bind(int idx)
{
	return sqlite3_bind_null(stmt_, idx);
}

int SqlLiteStateMnt::bind(const char* pName, int value)
{
	auto idx = sqlite3_bind_parameter_index(stmt_, pName);
	return bind(idx, value);
}

int SqlLiteStateMnt::bind(const char* pName, double value)
{
	auto idx = sqlite3_bind_parameter_index(stmt_, pName);
	return bind(idx, value);
}

int SqlLiteStateMnt::bind(const char* pName, long long int value)
{
	auto idx = sqlite3_bind_parameter_index(stmt_, pName);
	return bind(idx, value);
}

int SqlLiteStateMnt::bind(const char* pName, const char* value, CopySemantic::Enum  fcopy)
{
	auto idx = sqlite3_bind_parameter_index(stmt_, pName);
	return bind(idx, value, fcopy);
}

int SqlLiteStateMnt::bind(const char* pName, void const* value, int n, CopySemantic::Enum  fcopy)
{
	auto idx = sqlite3_bind_parameter_index(stmt_, pName);
	return bind(idx, value, n, fcopy);
}

int SqlLiteStateMnt::bind(const char* pName, std::string const& value, CopySemantic::Enum  fcopy)
{
	auto idx = sqlite3_bind_parameter_index(stmt_, pName);
	return bind(idx, value, fcopy);
}

int SqlLiteStateMnt::bind(const char* pName)
{
	auto idx = sqlite3_bind_parameter_index(stmt_, pName);
	return bind(idx);
}

// ----------------------------------------------------



SqlLiteCmd::BindStream::BindStream(SqlLiteCmd& cmd, int idx) :
	cmd_(cmd), 
	idx_(idx)
{
}

SqlLiteCmd::SqlLiteCmd(SqlLiteDb& db, char const* pStmt) :
	SqlLiteStateMnt(db, pStmt)
{
}

SqlLiteCmd::BindStream SqlLiteCmd::binder(int idx)
{
	return BindStream(*this, idx);
}

int SqlLiteCmd::execute(void)
{
	auto rc = step();
	if (rc == SQLITE_DONE) {
		rc = SQLITE_OK;
	}

	return rc;
}

int SqlLiteCmd::executeAll(void)
{
	auto rc = execute();
	if (rc != SQLITE_OK) {
		return rc;
	}

	char const* sql = tail_;

	while (std::strlen(sql) > 0) // sqlite3_complete() is broken.
	{
		sqlite3_stmt* old_stmt = stmt_;

		if ((rc = prepare_impl(sql)) != SQLITE_OK) {
			return rc;
		}

		if ((rc = sqlite3_transfer_bindings(old_stmt, stmt_)) != SQLITE_OK) {
			return rc;
		}

		finish_impl(old_stmt);

		if ((rc = execute()) != SQLITE_OK) {
			return rc;
		}

		sql = tail_;
	}

	return rc;
}


// ----------------------------------------------------


SqlLiteQuery::rows::getstream::getstream(rows* rws, int idx) : 
	rws_(rws), 
	idx_(idx)
{
}

SqlLiteQuery::rows::rows(sqlite3_stmt* pStmt) : 
	pStmt_(pStmt)
{
}

int SqlLiteQuery::rows::dataCount(void) const
{
	return sqlite3_data_count(pStmt_);
}

int SqlLiteQuery::rows::columnType(int idx) const
{
	return sqlite3_column_type(pStmt_, idx);
}

int SqlLiteQuery::rows::columnBytes(int idx) const
{
	return sqlite3_column_bytes(pStmt_, idx);
}

int SqlLiteQuery::rows::get(int idx, int) const
{
	return sqlite3_column_int(pStmt_, idx);
}

double SqlLiteQuery::rows::get(int idx, double) const
{
	return sqlite3_column_double(pStmt_, idx);
}

long long int SqlLiteQuery::rows::get(int idx, long long int) const
{
	return sqlite3_column_int64(pStmt_, idx);
}

char const* SqlLiteQuery::rows::get(int idx, char const*) const
{
	return reinterpret_cast<char const*>(sqlite3_column_text(pStmt_, idx));
}

std::string SqlLiteQuery::rows::get(int idx, std::string) const
{
	return get(idx, reinterpret_cast<const char*>(0));
}

void const* SqlLiteQuery::rows::get(int idx, void const*) const
{
	return sqlite3_column_blob(pStmt_, idx);
}


SqlLiteQuery::rows::getstream SqlLiteQuery::rows::getter(int idx)
{
	return getstream(this, idx);
}

SqlLiteQuery::query_iterator::query_iterator() : cmd_(0)
{
	rc_ = SQLITE_DONE;
}

SqlLiteQuery::query_iterator::query_iterator(SqlLiteQuery* cmd) : cmd_(cmd)
{
	rc_ = cmd_->step();
	if (rc_ != SQLITE_ROW && rc_ != SQLITE_DONE) {
		
	}
}

bool SqlLiteQuery::query_iterator::operator==(SqlLiteQuery::query_iterator const& other) const
{
	return rc_ == other.rc_;
}

bool SqlLiteQuery::query_iterator::operator!=(SqlLiteQuery::query_iterator const& other) const
{
	return rc_ != other.rc_;
}

SqlLiteQuery::query_iterator& SqlLiteQuery::query_iterator::operator++()
{
	rc_ = cmd_->step();
	if (rc_ != SQLITE_ROW && rc_ != SQLITE_DONE) {

	}
	return *this;
}

SqlLiteQuery::query_iterator::value_type SqlLiteQuery::query_iterator::operator*() const
{
	return rows(cmd_->stmt_);
}


SqlLiteQuery::SqlLiteQuery(SqlLiteDb& db, char const* stmt) :
	SqlLiteStateMnt(db, stmt)
{
}

int SqlLiteQuery::columnCount(void) const
{
	return sqlite3_column_count(stmt_);
}

char const* SqlLiteQuery::columnName(int idx) const
{
	return sqlite3_column_name(stmt_, idx);
}

char const* SqlLiteQuery::columnDecltype(int idx) const
{
	return sqlite3_column_decltype(stmt_, idx);
}


SqlLiteQuery::iterator SqlLiteQuery::begin(void)
{
	return query_iterator(this);
}

SqlLiteQuery::iterator SqlLiteQuery::end(void)
{
	return query_iterator();
}


// ----------------------------------------------------



SqlLiteTransaction::SqlLiteTransaction(SqlLiteDb& db, bool fcommit, bool freserve)
{
}

SqlLiteTransaction::~SqlLiteTransaction()
{
	if (pDb_) {
		auto rc = pDb_->execute(fcommit_ ? "COMMIT" : "ROLLBACK");
		if (rc != SQLITE_OK) {
			
		}
	}
}

int SqlLiteTransaction::commit(void)
{
	auto db = pDb_;
	pDb_ = nullptr;  // prevent execute on decon

	int rc = db->execute("COMMIT");
	return rc;
}

int SqlLiteTransaction::rollback(void)
{
	auto db = pDb_;
	pDb_ = nullptr; // prevent execute on decon

	int rc = db->execute("ROLLBACK");
	return rc;
}

X_NAMESPACE_END