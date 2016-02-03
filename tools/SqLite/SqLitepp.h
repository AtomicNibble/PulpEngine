#pragma once


struct sqlite3;
struct sqlite3_stmt;

X_NAMESPACE_BEGIN(sql)

class SqlLiteStateMnt;

template <class T>
struct convert {
	using to_int = int;
};

class DLL_EXPORT SqlLiteDb
{
	X_NO_COPY(SqlLiteDb);
	X_NO_ASSIGN(SqlLiteDb);

	friend class SqlLiteStateMnt;

public:
	typedef int64_t RowId;

public:
	SqlLiteDb();
	~SqlLiteDb();

	bool connect(const char* pDb);
	bool disconnect(void);

	int enableForeignKeys(bool enable = true);
	int enableTriggers(bool enable = true);
	int enableExtendedResultCodes(bool enable = true);

	RowId lastInsertRowid(void) const;

	int errorCode(void) const;
	const char* errorMsg(void) const;

	bool execute(const char* sql);
	bool executeFmt(const char* sql, ...);

private:
	sqlite3* db_;
};


class DLL_EXPORT SqlLiteStateMnt
{
	X_NO_COPY(SqlLiteStateMnt);
	X_NO_ASSIGN(SqlLiteStateMnt);


protected:
	explicit SqlLiteStateMnt(SqlLiteDb& db, const char* pStmt = nullptr);
	~SqlLiteStateMnt();

	int prepare_impl(const char* pStmt);
	int finish_impl(sqlite3_stmt* pStmt);

public:
	X_DECLARE_ENUM(CopySemantic)(COPY,NOCOPY);

public:
	int prepare(const char* pStmt);
	int finish(void);

	int step(void);
	int reset(void);

	int bind(int idx, int value);
	int bind(int idx, double value);
	int bind(int idx, long long int value);
	int bind(int idx, const char* value, CopySemantic::Enum fcopy);
	int bind(int idx, void const* value, int n, CopySemantic::Enum fcopy);
	int bind(int idx, std::string const& value, CopySemantic::Enum fcopy);
	int bind(int idx);

	int bind(const char* pName, int value);
	int bind(const char* pName, double value);
	int bind(const char* pName, long long int value);
	int bind(const char* pName, const char* value, CopySemantic::Enum fcopy);
	int bind(const char* pName, void const* value, int n, CopySemantic::Enum fcopy);
	int bind(const char* pName, std::string const& value, CopySemantic::Enum fcopy);
	int bind(const char* pName);

protected:
	SqlLiteDb& db_;
	sqlite3_stmt* stmt_;
	const char* tail_;
};

class DLL_EXPORT SqlLiteCmd : public SqlLiteStateMnt
{
public:
	class BindStream
	{
	public:
		BindStream(SqlLiteCmd& cmd, int idx);

		template <class T>
		X_INLINE SqlLiteCmd& operator << (T value)
		{
			auto rc = cmd_.bind(idx_, value);
			if (rc != SQLITE_OK) {
				//
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
	~SqlLiteCmd() = default;

	BindStream binder(int idx = 1);

	int execute(void);
	int executeAll(void);
};



class SqlLiteQuery : public SqlLiteStateMnt
{
public:
	class rows
	{
	public:
		class getstream
		{
		public:
			getstream(rows* rws, int idx);

			template <class T>
			X_INLINE getstream& operator >> (T& value) {
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
		int columnType(int idx) const;
		int columnBytes(int idx) const;

		template <class T> 
		T get(int idx) const {
			return get(idx, T());
		}

		template <class... Ts>
		X_INLINE std::tuple<Ts...> getColumns(typename convert<Ts>::to_int... idxs) const {
			return std::make_tuple(get(idxs, Ts())...);
		}

		getstream getter(int idx = 0);

	private:
		int get(int idx, int) const;
		double get(int idx, double) const;
		long long int get(int idx, long long int) const;
		char const* get(int idx, char const*) const;
		std::string get(int idx, std::string) const;
		void const* get(int idx, void const*) const;

	private:
		sqlite3_stmt* pStmt_;
	};

	class query_iterator
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
		SqlLiteQuery* cmd_;
		int rc_;
	};

public:
	using iterator = query_iterator;

public:
	explicit SqlLiteQuery(SqlLiteDb& db, char const* pStmt = nullptr);
	~SqlLiteQuery() = default;

	int columnCount(void) const;

	const char* columnName(int idx) const;
	const char* columnDecltype(int idx) const;

	iterator begin(void);
	iterator end(void);
};

class DLL_EXPORT SqlLiteTransaction
{
	X_NO_COPY(SqlLiteTransaction);
	X_NO_ASSIGN(SqlLiteTransaction);


public:
	explicit SqlLiteTransaction(SqlLiteDb& db, bool fcommit = false, bool freserve = false);
	~SqlLiteTransaction();

	int commit(void);
	int rollback(void);

private:
	SqlLiteDb* pDb_;
	bool fcommit_;
};


X_NAMESPACE_END