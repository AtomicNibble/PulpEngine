#pragma once

#include <Util\EnumMacros.h>

#include <Traits\FunctionTraits.h>

struct sqlite3;
struct sqlite3_stmt;

X_NAMESPACE_BEGIN(sql)

class SqlLiteStateMnt;

template<class T>
struct convert
{
    using to_int = int;
};

class null_type
{
};
extern null_type ignore;

struct Result
{
    enum Enum
    {
        OK = 0,
        ERROR = 1,
        INTERNAL = 2,
        PERM = 3,
        ABORT = 4,
        BUSY = 5,
        LOCKED = 6,
        NOMEM = 7,
        READONLY = 8,
        INTERRUPT = 9,
        IOERR = 10,
        CORRUPT = 11,
        NOTFOUND = 12,
        FULL = 13,
        CANTOPEN = 14,
        PROTOCOL = 15,
        EMPTY = 16,
        SCHEMA = 17,
        TOOBIG = 18,
        CONSTRAINT = 19,
        MISMATCH = 20,
        MISUSE = 21,
        NOLFS = 22,
        AUTH = 23,
        FORMAT = 24,
        RANGE = 25,
        NOTADB = 26,
        NOTICE = 27,
        WARNING = 28,
        ROW = 100,
        DONE = 101,
    };
};

struct ColumType
{
    enum Enum
    {
        INTEGER = 1, // SQLITE_INTEGER,
        FLOAT = 2,   // SQLITE_FLOAT,
        TEXT = 3,    // SQLITE_TEXT,
        BLOB = 4,    // SQLITE_BLOB,
        SNULL = 5,   // SQLITE_NULL
    };

    static const char* ToString(Enum type);
};

X_DECLARE_ENUM(ThreadMode)
(
    UNSPECIFIED,
    SINGLE,    // one thread only
    MULTI,     // one thread per DB connection
    SERIALIZED // many threads one db.
);

X_DECLARE_FLAGS(OpenFlag)
(
    WRITE,
    CREATE
);

typedef Flags<OpenFlag> OpenFlags;

X_DECLARE_FLAG_OPERATORS(OpenFlags);


class DLL_EXPORT SqlLiteDb
{
    X_NO_COPY(SqlLiteDb);
    X_NO_ASSIGN(SqlLiteDb);

    friend class SqlLiteStateMnt;

private:
    static ThreadMode::Enum currentThreadMode;

public:
    typedef int64_t RowId;

    typedef core::traits::Function<int(int)> BusyHandler;
    typedef core::traits::Function<int(void)> CommmitHandler;
    typedef core::traits::Function<void(void)> RollBackHandler;
    typedef core::traits::Function<int(int, const char*, const char*, long long int)> UpdateHandler;
    typedef core::traits::Function<int(int, const char*, const char*, const char*, const char*)> AuthorizeHandler;

public:
    SqlLiteDb();
    SqlLiteDb(SqlLiteDb&&);
    ~SqlLiteDb();

    SqlLiteDb& operator=(SqlLiteDb&&);

    static bool setThreadMode(ThreadMode::Enum threadMode);

    bool connect(const char* pDb, OpenFlags flags);
    bool disconnect(void);

    Result::Enum enableForeignKeys(bool enable);
    Result::Enum enableTriggers(bool enable);
    Result::Enum enableExtendedResultCodes(bool enable);

    RowId lastInsertRowid(void) const;
    int32_t numChangesFromLastStmt(void) const;
    int32_t numChangesSinceDBOpen(void) const;

    Result::Enum errorCode(void) const;
    const char* errorMsg(void) const;

    bool execute(const char* sql);
    bool executeFmt(const char* sql, ...);

    Result::Enum executeRes(const char* sql);
    Result::Enum executeFmtRes(const char* sql, ...);

    Result::Enum setBusyTimeout(int32_t ms);

    void setBusyHandler(BusyHandler::Pointer h);
    void setCommitHandler(CommmitHandler::Pointer h);
    void setRollBackHandler(RollBackHandler::Pointer h);
    void setUpdateHandler(UpdateHandler::Pointer h);
    void setAuthorizeHandler(AuthorizeHandler::Pointer h);

private:
    static Result::Enum setConfig(int32_t config);
    static int32_t defaultBusyHandler(int32_t count);

private:
    sqlite3* db_;

    BusyHandler::Pointer bh_;
    CommmitHandler::Pointer ch_;
    RollBackHandler::Pointer rh_;
    UpdateHandler::Pointer uh_;
    AuthorizeHandler::Pointer ah_;
};

class DLL_EXPORT SqlLiteStateMnt
{
    X_NO_COPY(SqlLiteStateMnt);
    X_NO_ASSIGN(SqlLiteStateMnt);

protected:
    explicit SqlLiteStateMnt(SqlLiteDb& db, const char* pStmt = nullptr);
    virtual ~SqlLiteStateMnt();

    Result::Enum prepare_impl(const char* pStmt);
    Result::Enum finish_impl(sqlite3_stmt* pStmt);

public:
    X_DECLARE_ENUM(CopySemantic)
    (COPY, NOCOPY);

public:
    Result::Enum prepare(const char* pStmt);
    Result::Enum finish(void);

    Result::Enum step(void);
    Result::Enum reset(void);

    Result::Enum bind(int idx, int value);
    Result::Enum bind(int idx, double value);
    Result::Enum bind(int idx, long long int value);
    Result::Enum bind(int idx, const char* value, CopySemantic::Enum fcopy = CopySemantic::NOCOPY);
    Result::Enum bind(int idx, const char* value, size_t length, CopySemantic::Enum fcopy = CopySemantic::NOCOPY);
    Result::Enum bind(int idx, void const* value, int n, CopySemantic::Enum fcopy = CopySemantic::NOCOPY);
    Result::Enum bind(int idx, std::string const& value, CopySemantic::Enum fcopy = CopySemantic::NOCOPY);
    Result::Enum bind(int idx);
    Result::Enum bind(int idx, null_type);

    template<typename T, class = typename std::enable_if<std::is_enum<T>::value>::type>
    X_INLINE Result::Enum bind(int idx, T value)
    {
        return bind(idx, static_cast<int>(value));
    }

    Result::Enum bind(const char* pName, int value);
    Result::Enum bind(const char* pName, double value);
    Result::Enum bind(const char* pName, long long int value);
    Result::Enum bind(const char* pName, const char* value, CopySemantic::Enum fcopy = CopySemantic::NOCOPY);
    Result::Enum bind(const char* pName, const char* value, size_t length, CopySemantic::Enum fcopy = CopySemantic::NOCOPY);
    Result::Enum bind(const char* pName, void const* value, int n, CopySemantic::Enum fcopy = CopySemantic::NOCOPY);
    Result::Enum bind(const char* pName, std::string const& value, CopySemantic::Enum fcopy = CopySemantic::NOCOPY);
    Result::Enum bind(const char* pName);
    Result::Enum bind(const char* pName, null_type);

    template<typename T, class = typename std::enable_if<std::is_enum<T>::value>::type>
    X_INLINE Result::Enum bind(const char* pName, T value)
    {
        return bind(pName, static_cast<int>(value));
    }

protected:
    SqlLiteDb& db_;
    sqlite3_stmt* pStmt_;
    const char* pTail_;
};

class DLL_EXPORT SqlLiteCmd : public SqlLiteStateMnt
{
public:
    class DLL_EXPORT BindStream
    {
    public:
        BindStream(SqlLiteCmd& cmd, int idx);

        template<class T>
        X_INLINE BindStream& operator<<(T value)
        {
            auto rc = cmd_.bind(idx_, value);
            if (rc != Result::OK) {
                // add error handling?
                X_ASSERT_UNREACHABLE();
            }
            ++idx_;
            return *this;
        }

    private:
        SqlLiteCmd& cmd_;
        int idx_;
    };

public:
    explicit SqlLiteCmd(SqlLiteDb& db, const char* pStmt = nullptr);
    ~SqlLiteCmd() X_OVERRIDE;

    BindStream binder(int idx = 1);

    Result::Enum execute(void);
    Result::Enum executeAll(void);
};

class DLL_EXPORT SqlLiteQuery : public SqlLiteStateMnt
{
public:
    class DLL_EXPORT rows
    {
    public:
        class DLL_EXPORT getstream
        {
        public:
            getstream(rows* rws, int idx);

            template<class T>
            X_INLINE getstream& operator>>(T& value)
            {
                value = rws_->get(idx_, T());
                ++idx_;
                return *this;
            }

        private:
            rows* rws_;
            int idx_;
        };

        explicit rows(sqlite3_stmt* pStmt);

        int dataCount(void) const;
        ColumType::Enum columnType(int idx) const;
        int columnBytes(int idx) const;

        template<class T>
        T get(int idx) const
        {
            return get(idx, T());
        }

        template<class... Ts>
        X_INLINE std::tuple<Ts...> getColumns(typename convert<Ts>::to_int... idxs) const
        {
            return std::make_tuple(get(idxs, Ts())...);
        }

        getstream getter(int idx = 0);

    private:
        int get(int idx, int) const;
        double get(int idx, double) const;
        long long int get(int idx, long long int) const;
        const char* get(int idx, const char*) const;
        std::string get(int idx, std::string) const;
        void const* get(int idx, void const*) const;
        null_type get(int idx, null_type) const;

    private:
        sqlite3_stmt* pStmt_;
    };

    class DLL_EXPORT query_iterator
        : public std::iterator<std::input_iterator_tag, rows>
    {
    public:
        query_iterator();
        explicit query_iterator(SqlLiteQuery* pCmd);

        bool operator==(query_iterator const&) const;
        bool operator!=(query_iterator const&) const;

        query_iterator& operator++();

        value_type operator*() const;

    private:
        SqlLiteQuery* pCmd_;
        int rc_;
    };

public:
    using iterator = query_iterator;

public:
    explicit SqlLiteQuery(SqlLiteDb& db, char const* pStmt = nullptr);
    ~SqlLiteQuery() X_OVERRIDE;

    int columnCount(void) const;

    const char* columnName(int idx) const;
    const char* columnDecltype(int idx) const;

    iterator begin(void);
    iterator end(void);
};

class DLL_EXPORT SqlLiteTransactionBase
{
protected:
    SqlLiteTransactionBase() = default;
    ~SqlLiteTransactionBase() = default;

    X_NO_COPY(SqlLiteTransactionBase);
    X_NO_ASSIGN(SqlLiteTransactionBase);
};

class DLL_EXPORT SqlLiteTransaction : public SqlLiteTransactionBase
{
    X_NO_COPY(SqlLiteTransaction);
    X_NO_ASSIGN(SqlLiteTransaction);

public:
    explicit SqlLiteTransaction(SqlLiteDb& db,
        bool fcommit = false,   // if true COMMIT's on decon otherwise ROLLBACK
        bool freserve = false); // if true BEGIN IMMEDIATE else BEGIN
    ~SqlLiteTransaction();

    Result::Enum commit(void);
    Result::Enum rollback(void);

private:
    SqlLiteDb* pDb_;
    bool fcommit_;
};

X_NAMESPACE_END