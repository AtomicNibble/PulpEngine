#include "stdafx.h"
#include "SqLitepp.h"

#include <memory>

#if X_COMPILER_CLANG

#define SQLITE_DISABLE_INTRINSIC 1

#ifdef _MSC_VER
#undef _MSC_VER
#endif

#endif

#include <../../3rdparty/source/sqlite-3.19.2/sqlite3.h>

X_NAMESPACE_BEGIN(sql)

namespace
{
    int32_t busyHandler_impl(void* p, int32_t cnt)
    {
        auto h = static_cast<SqlLiteDb::BusyHandler::Pointer*>(p);
        return (*h)(cnt);
    }

    int32_t commitHook_impl(void* p)
    {
        auto h = static_cast<SqlLiteDb::CommmitHandler::Pointer*>(p);
        return (*h)();
    }

    void rollbackHook_impl(void* p)
    {
        auto h = static_cast<SqlLiteDb::RollBackHandler::Pointer*>(p);
        (*h)();
    }

    void updateHook_impl(void* p, int32_t opcode, const char* dbname, const char* tablename, long long int rowid)
    {
        auto h = static_cast<SqlLiteDb::UpdateHandler::Pointer*>(p);
        (*h)(opcode, dbname, tablename, rowid);
    }

    int32_t authorizer_impl(void* p, int32_t evcode, const char* p1, const char* p2, const char* dbname, const char* tvname)
    {
        auto h = static_cast<SqlLiteDb::AuthorizeHandler::Pointer*>(p);
        return (*h)(evcode, p1, p2, dbname, tvname);
    }

    struct SqlitePrintfDeleter
    {
        void operator()(char* pStr)
        {
            sqlite3_free(pStr);
        }
    };

} // namespace

const char* ColumType::ToString(Enum type)
{
    switch (type) {
        case INTEGER:
            return "Int";
        case FLOAT:
            return "Float";
        case TEXT:
            return "Text";
        case BLOB:
            return "Blob";
        case SNULL:
            return "NULL";

        default:
            X_NO_SWITCH_DEFAULT_ASSERT;
    }

    return "<ukn>";
}

// --------------------------------------

SqlLiteDb::SqlLiteDb() :
    db_(nullptr),
    bh_(nullptr),
    ch_(nullptr),
    rh_(nullptr),
    uh_(nullptr),
    ah_(nullptr)
{
}

SqlLiteDb::SqlLiteDb(SqlLiteDb&& oth) :
    db_(std::move(oth.db_)),
    bh_(std::move(oth.bh_)),
    ch_(std::move(oth.ch_)),
    rh_(std::move(oth.rh_)),
    uh_(std::move(oth.uh_)),
    ah_(std::move(oth.ah_))
{
    oth.db_ = nullptr;
}

SqlLiteDb::~SqlLiteDb()
{
    disconnect();
}

SqlLiteDb& SqlLiteDb::operator=(SqlLiteDb&& oth)
{
    db_ = std::move(oth.db_);
    oth.db_ = nullptr;

    bh_ = std::move(oth.bh_);
    ch_ = std::move(oth.ch_);
    rh_ = std::move(oth.rh_);
    uh_ = std::move(oth.uh_);
    ah_ = std::move(oth.ah_);
    return *this;
}

bool SqlLiteDb::setThreadMode(ThreadMode::Enum threadMode)
{
    static ThreadMode::Enum currentThreadMode = ThreadMode::SERIALIZED;

    X_ASSERT(sqlite3_threadsafe() != 0, "Sqlite was not compiled to support thread safe access")();

    if (threadMode == currentThreadMode) {
        return true;
    }

    currentThreadMode = threadMode;

    Result::Enum res = Result::OK;
    if (threadMode == ThreadMode::SINGLE) {
        res = setConfig(SQLITE_CONFIG_SINGLETHREAD);
    }
    else if (threadMode == ThreadMode::MULTI) {
        res = setConfig(SQLITE_CONFIG_MULTITHREAD);
    }
    else if (threadMode == ThreadMode::SERIALIZED) {
        // why are you using this mode you twat.
        /// I actually use this mode now :(
        /// but I don't use it for performance reasons.
        /// I use it so the DB can be accssed from the UI thread and background thread that performs converting in AssetManager.
        /// the standalone converter still uses single threaded mode.
        //X_WARNING("SqlLiteDb", "Enabling multi threading for single db connection, are you sure you want this? (answer is no.)");
        res = setConfig(SQLITE_CONFIG_SERIALIZED);
    }
    else {
        X_ASSERT_UNREACHABLE();
    }

    if (res != Result::OK) {
        X_ERROR("SqlLiteDb", "Failed to set threading mode: %d", res);
        return false;
    }
    return true;
}

bool SqlLiteDb::connect(const char* pDb, ThreadMode::Enum threadMode)
{
    X_ASSERT_NOT_NULL(pDb);

    if (!setThreadMode(threadMode)) {
        X_WARNING("AssetDB", "Failed to set thread mode");
        return false;
    }

    if (!disconnect()) {
        X_ERROR("SqlLiteDb", "Failed to disconeect beofre conencting to new db: \"%s\"", pDb);
        return false;
    }

    int ret;

    if (Result::OK != (ret = sqlite3_initialize())) {
        X_ERROR("SqlLiteDb", "Failed to initialize library: %d", ret);
        return false;
    }

    // open connection to a DB
    if (Result::OK != (ret = sqlite3_open_v2(pDb, &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr))) {
        X_ERROR("SqlLiteDb", "Failed to open conn: %d", ret);
        return false;
    }

    setBusyHandler(defaultBusyHandler);

    return true;
}

bool SqlLiteDb::disconnect(void)
{
    if (db_) {
        int ret, ret2;
        if (Result::OK != (ret = sqlite3_close_v2(db_))) {
            X_ERROR("SqlLiteDb", "Failed to close db");
        }

        if (Result::OK != (ret2 = sqlite3_shutdown())) {
            X_ERROR("SqlLiteDb", "Failed to shutdown sqLite");
        }

        db_ = nullptr;
        return ret == Result::OK && ret2 == Result::OK;
    }
    return true;
}

Result::Enum SqlLiteDb::enableForeignKeys(bool enable)
{
    return static_cast<Result::Enum>(sqlite3_db_config(db_, SQLITE_DBCONFIG_ENABLE_FKEY, enable ? 1 : 0, nullptr));
}

Result::Enum SqlLiteDb::enableTriggers(bool enable)
{
    return static_cast<Result::Enum>(sqlite3_db_config(db_, SQLITE_DBCONFIG_ENABLE_TRIGGER, enable ? 1 : 0, nullptr));
}

Result::Enum SqlLiteDb::enableExtendedResultCodes(bool enable)
{
    return static_cast<Result::Enum>(sqlite3_extended_result_codes(db_, enable ? 1 : 0));
}

SqlLiteDb::RowId SqlLiteDb::lastInsertRowid(void) const
{
    return sqlite3_last_insert_rowid(db_);
}

int32_t SqlLiteDb::numChangesFromLastStmt(void) const
{
    return sqlite3_changes(db_);
}

int32_t SqlLiteDb::numChangesSinceDBOpen(void) const
{
    return sqlite3_total_changes(db_);
}

Result::Enum SqlLiteDb::errorCode(void) const
{
    return static_cast<Result::Enum>(sqlite3_errcode(db_));
}

const char* SqlLiteDb::errorMsg(void) const
{
    return sqlite3_errmsg(db_);
}

bool SqlLiteDb::execute(const char* sql)
{
    Result::Enum res = static_cast<Result::Enum>(sqlite3_exec(db_, sql, 0, 0, 0));
    if (res == Result::OK) {
        return true;
    }

    X_ERROR("SqlDb", "execue failed for \"%s\" err(%i): %s", sql, res, errorMsg());
    return false;
}

bool SqlLiteDb::executeFmt(const char* sql, ...)
{
    va_list ap;
    va_start(ap, sql);
    std::unique_ptr<char, SqlitePrintfDeleter> msql(sqlite3_vmprintf(sql, ap));
    va_end(ap);

    return execute(msql.get());
}

Result::Enum SqlLiteDb::executeRes(const char* sql)
{
    Result::Enum res = static_cast<Result::Enum>(sqlite3_exec(db_, sql, 0, 0, 0));
    if (res == Result::OK) {
        return res;
    }

    X_ERROR("SqlDb", "execue failed for \"%s\" err(%i): %s", sql, res, errorMsg());
    return res;
}

Result::Enum SqlLiteDb::executeFmtRes(const char* sql, ...)
{
    va_list ap;
    va_start(ap, sql);
    std::unique_ptr<char, SqlitePrintfDeleter> msql(sqlite3_vmprintf(sql, ap));
    va_end(ap);

    return executeRes(msql.get());
}

Result::Enum SqlLiteDb::setBusyTimeout(int32_t ms)
{
    return static_cast<Result::Enum>(sqlite3_busy_timeout(db_, ms));
}

void SqlLiteDb::setBusyHandler(BusyHandler::Pointer h)
{
    bh_ = h;
    sqlite3_busy_handler(db_, bh_ ? busyHandler_impl : 0, &bh_);
}

void SqlLiteDb::setCommitHandler(CommmitHandler::Pointer h)
{
    ch_ = h;
    sqlite3_commit_hook(db_, ch_ ? commitHook_impl : 0, &ch_);
}

void SqlLiteDb::setRollBackHandler(RollBackHandler::Pointer h)
{
    rh_ = h;
    sqlite3_rollback_hook(db_, rh_ ? rollbackHook_impl : 0, &rh_);
}

void SqlLiteDb::setUpdateHandler(UpdateHandler::Pointer h)
{
    uh_ = h;
    sqlite3_update_hook(db_, uh_ ? updateHook_impl : 0, &uh_);
}

void SqlLiteDb::setAuthorizeHandler(AuthorizeHandler::Pointer h)
{
    ah_ = h;
    sqlite3_set_authorizer(db_, ah_ ? authorizer_impl : 0, &ah_);
}

Result::Enum SqlLiteDb::setConfig(int32_t config)
{
    return static_cast<Result::Enum>(sqlite3_config(config));
}

int32_t SqlLiteDb::defaultBusyHandler(int32_t count)
{
    X_WARNING("SqlLiteDb", "Busy handler called %" PRIi32 " times", count);
    return 0;
}
// ----------------------------------------------------

SqlLiteStateMnt::SqlLiteStateMnt(SqlLiteDb& db, const char* pStmt) :
    db_(db),
    pStmt_(nullptr),
    pTail_(nullptr)
{
    if (pStmt) {
        auto rc = prepare(pStmt);
        if (rc != Result::OK) {
            X_ERROR("SqlDb", "statement prepare failed for \"%s\" err(%i): \"%s\"", pStmt, rc, db.errorMsg());
        }
    }
}

SqlLiteStateMnt::~SqlLiteStateMnt()
{
    auto rc = finish();
    if (rc != Result::OK) {
        X_ERROR("SqlDb", "statement finish failed err(%i): \"%s\"", rc, db_.errorMsg());
    }
}

Result::Enum SqlLiteStateMnt::prepare(const char* pStmt)
{
    auto rc = finish();
    if (rc != Result::OK) {
        X_ERROR("SqlDb", "prepare err(%i): \"%s\"", rc, db_.errorMsg());
        return rc;
    }

    return prepare_impl(pStmt);
}

Result::Enum SqlLiteStateMnt::prepare_impl(const char* pStmt)
{
    return static_cast<Result::Enum>(sqlite3_prepare_v2(
        db_.db_,
        pStmt,
        safe_static_cast<int32, size_t>(std::strlen(pStmt)),
        &pStmt_,
        &pTail_));
}

Result::Enum SqlLiteStateMnt::finish(void)
{
    auto rc = Result::OK;
    if (pStmt_) {
        rc = static_cast<Result::Enum>(finish_impl(pStmt_));
        pStmt_ = nullptr;
    }

    if (rc != Result::OK) {
        X_ERROR("SqlDb", "finish err(%i): \"%s\"", rc, db_.errorMsg());
    }

    pTail_ = nullptr;
    return rc;
}

Result::Enum SqlLiteStateMnt::finish_impl(sqlite3_stmt* pStmt)
{
    return static_cast<Result::Enum>(sqlite3_finalize(pStmt));
}

Result::Enum SqlLiteStateMnt::step(void)
{
    return static_cast<Result::Enum>(sqlite3_step(pStmt_));
}

Result::Enum SqlLiteStateMnt::reset(void)
{
    return static_cast<Result::Enum>(sqlite3_reset(pStmt_));
}

Result::Enum SqlLiteStateMnt::bind(int idx, int value)
{
    Result::Enum result = static_cast<Result::Enum>(sqlite3_bind_int(pStmt_, idx, value));
    if (result != Result::OK) {
        X_ERROR("SqLite", "Bind <int32_t> failed. err(%i): \"%s\"", result, db_.errorMsg());
    }
    return result;
}

Result::Enum SqlLiteStateMnt::bind(int idx, double value)
{
    Result::Enum result = static_cast<Result::Enum>(sqlite3_bind_double(pStmt_, idx, value));
    if (result != Result::OK) {
        X_ERROR("SqLite", "Bind <double> failed. err(%i): \"%s\"", result, db_.errorMsg());
    }
    return result;
}

Result::Enum SqlLiteStateMnt::bind(int idx, long long int value)
{
    Result::Enum result = static_cast<Result::Enum>(sqlite3_bind_int64(pStmt_, idx, value));
    if (result != Result::OK) {
        X_ERROR("SqLite", "Bind <int64_t> failed. err(%i): \"%s\"", result, db_.errorMsg());
    }
    return result;
}

Result::Enum SqlLiteStateMnt::bind(int idx, const char* value, CopySemantic::Enum fcopy)
{
    Result::Enum res = static_cast<Result::Enum>(sqlite3_bind_text(
        pStmt_,
        idx,
        value,
        safe_static_cast<int32, size_t>(std::strlen(value)),
        fcopy == CopySemantic::COPY ? SQLITE_TRANSIENT : SQLITE_STATIC));

    if (res != Result::OK) {
        X_ERROR("", "bind(%i)-> \"%s\" failed. err(%i): \"%s\"",
            idx, value, res, db_.errorMsg());
    }

    return res;
}

Result::Enum SqlLiteStateMnt::bind(int idx, const void* value, int n, CopySemantic::Enum fcopy)
{
    Result::Enum res = static_cast<Result::Enum>(sqlite3_bind_blob(
        pStmt_,
        idx,
        value,
        n,
        fcopy == CopySemantic::COPY ? SQLITE_TRANSIENT : SQLITE_STATIC));

    if (res != Result::OK) {
        X_ERROR("", "bind(%i)-> %p failed. err(%i): \"%s\"",
            idx, value, res, db_.errorMsg());
    }

    return res;
}

Result::Enum SqlLiteStateMnt::bind(int idx, std::string const& value, CopySemantic::Enum fcopy)
{
    Result::Enum res = static_cast<Result::Enum>(sqlite3_bind_text(
        pStmt_,
        idx,
        value.c_str(),
        safe_static_cast<int32, size_t>(value.size()),
        fcopy == CopySemantic::COPY ? SQLITE_TRANSIENT : SQLITE_STATIC));

    if (res != Result::OK) {
        X_ERROR("", "bind(%i)-> \"%s\" failed. err(%i): \"%s\"",
            idx, value.c_str(), res, db_.errorMsg());
    }

    return res;
}

Result::Enum SqlLiteStateMnt::bind(int idx)
{
    return static_cast<Result::Enum>(sqlite3_bind_null(pStmt_, idx));
}

Result::Enum SqlLiteStateMnt::bind(int idx, null_type)
{
    return bind(idx);
}

Result::Enum SqlLiteStateMnt::bind(const char* pName, int value)
{
    auto idx = sqlite3_bind_parameter_index(pStmt_, pName);
    return bind(idx, value);
}

Result::Enum SqlLiteStateMnt::bind(const char* pName, double value)
{
    auto idx = sqlite3_bind_parameter_index(pStmt_, pName);
    return bind(idx, value);
}

Result::Enum SqlLiteStateMnt::bind(const char* pName, long long int value)
{
    auto idx = sqlite3_bind_parameter_index(pStmt_, pName);
    return bind(idx, value);
}

Result::Enum SqlLiteStateMnt::bind(const char* pName, const char* value, CopySemantic::Enum fcopy)
{
    auto idx = sqlite3_bind_parameter_index(pStmt_, pName);
    return bind(idx, value, fcopy);
}

Result::Enum SqlLiteStateMnt::bind(const char* pName, void const* value, int n, CopySemantic::Enum fcopy)
{
    auto idx = sqlite3_bind_parameter_index(pStmt_, pName);
    return bind(idx, value, n, fcopy);
}

Result::Enum SqlLiteStateMnt::bind(const char* pName, std::string const& value, CopySemantic::Enum fcopy)
{
    auto idx = sqlite3_bind_parameter_index(pStmt_, pName);
    return bind(idx, value, fcopy);
}

Result::Enum SqlLiteStateMnt::bind(const char* pName)
{
    auto idx = sqlite3_bind_parameter_index(pStmt_, pName);
    return bind(idx);
}

Result::Enum SqlLiteStateMnt::bind(const char* pName, null_type)
{
    return bind(pName);
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

SqlLiteCmd::~SqlLiteCmd()
{
}

SqlLiteCmd::BindStream SqlLiteCmd::binder(int idx)
{
    return BindStream(*this, idx);
}

Result::Enum SqlLiteCmd::execute(void)
{
    auto rc = step();
    if (rc == Result::DONE) {
        rc = Result::OK;
    }

    if (rc != Result::OK) {
        X_ERROR("SqlDb", "query step err(%i): \"%s\"", rc, db_.errorMsg());
    }

    return rc;
}

Result::Enum SqlLiteCmd::executeAll(void)
{
    auto rc = execute();
    if (rc != Result::OK) {
        X_ERROR("SqlDb", "executeAll err(%i): \"%s\"", rc, db_.errorMsg());
        return rc;
    }

    char const* sql = pTail_;

    while (std::strlen(sql) > 0) // sqlite3_complete() is broken.
    {
        sqlite3_stmt* old_stmt = pStmt_;

        if ((rc = prepare_impl(sql)) != Result::OK) {
            X_ERROR("SqlDb", "executeAll err(%i): \"%s\"", rc, db_.errorMsg());
            return rc;
        }

        if ((rc = static_cast<Result::Enum>(sqlite3_transfer_bindings(old_stmt, pStmt_))) != Result::OK) {
            X_ERROR("SqlDb", "executeAll err(%i): \"%s\"", rc, db_.errorMsg());
            return rc;
        }

        finish_impl(old_stmt);

        if ((rc = execute()) != Result::OK) {
            X_ERROR("SqlDb", "executeAll err(%i): \"%s\"", rc, db_.errorMsg());
            return rc;
        }

        sql = pTail_;
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

ColumType::Enum SqlLiteQuery::rows::columnType(int idx) const
{
    return static_cast<ColumType::Enum>(sqlite3_column_type(pStmt_, idx));
}

int SqlLiteQuery::rows::columnBytes(int idx) const
{
    return sqlite3_column_bytes(pStmt_, idx);
}

int SqlLiteQuery::rows::get(int idx, int) const
{
    const int res = sqlite3_column_int(pStmt_, idx);
    if (res == 0) {
        ColumType::Enum type = static_cast<ColumType::Enum>(columnType(idx));
        if (type != ColumType::INTEGER) {
            X_ERROR("SqlDb", "Index is is not a int: %i type: %s(%i)", idx, ColumType::ToString(type), type);
        }
    }
    return res;
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

SqlLiteQuery::query_iterator::query_iterator() :
    pCmd_(0)
{
    rc_ = Result::DONE;
}

SqlLiteQuery::query_iterator::query_iterator(SqlLiteQuery* cmd) :
    pCmd_(cmd)
{
    // if the query failed, make this same as end.
    if (!cmd->pStmt_) {
        pCmd_ = nullptr;
        rc_ = Result::DONE;
        return;
    }

    rc_ = pCmd_->step();
    if (rc_ != Result::ROW && rc_ != Result::DONE) {
        X_ERROR("SqlDb", "query step err(%i)", rc_);
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
    rc_ = pCmd_->step();
    if (rc_ != Result::ROW && rc_ != Result::DONE) {
        X_ERROR("SqlDb", "query step err(%i)", rc_);
    }
    return *this;
}

SqlLiteQuery::query_iterator::value_type SqlLiteQuery::query_iterator::operator*() const
{
    return rows(pCmd_->pStmt_);
}

SqlLiteQuery::SqlLiteQuery(SqlLiteDb& db, char const* stmt) :
    SqlLiteStateMnt(db, stmt)
{
}

SqlLiteQuery::~SqlLiteQuery()
{
}

int SqlLiteQuery::columnCount(void) const
{
    return sqlite3_column_count(pStmt_);
}

char const* SqlLiteQuery::columnName(int idx) const
{
    return sqlite3_column_name(pStmt_, idx);
}

char const* SqlLiteQuery::columnDecltype(int idx) const
{
    return sqlite3_column_decltype(pStmt_, idx);
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

SqlLiteTransaction::SqlLiteTransaction(SqlLiteDb& db, bool fcommit, bool freserve) :
    pDb_(&db),
    fcommit_(fcommit)
{
    pDb_->execute(freserve ? "BEGIN IMMEDIATE" : "BEGIN");
}

SqlLiteTransaction::~SqlLiteTransaction()
{
    if (pDb_) {
        auto rc = pDb_->executeRes(fcommit_ ? "COMMIT" : "ROLLBACK");
        if (rc != Result::OK) {
            X_ERROR("SqlDb", "transaction err(%i): \"%s\"", rc, pDb_->errorMsg());
        }
    }
}

Result::Enum SqlLiteTransaction::commit(void)
{
    auto db = pDb_;
    pDb_ = nullptr; // prevent execute on decon

    auto rc = db->executeRes("COMMIT");
    if (rc != Result::OK) {
        X_ERROR("SqlDb", "transaction commit err(%i): \"%s\"", rc, pDb_->errorMsg());
    }
    return rc;
}

Result::Enum SqlLiteTransaction::rollback(void)
{
    auto db = pDb_;
    pDb_ = nullptr; // prevent execute on decon

    auto rc = db->executeRes("ROLLBACK");
    if (rc != Result::OK) {
        X_ERROR("SqlDb", "transaction rollback err(%i): \"%s\"", rc, pDb_->errorMsg());
    }
    return rc;
}

X_NAMESPACE_END