#include <tables/utils.hpp>

#include <iostream>
#include <stdexcept>

#include <fmt/format.h>

#include <types.hpp>

namespace {

namespace details {

template <typename T> requires std::is_integral_v<T>
T GetQueryResultImpl(sqlite3_stmt* stmt, int pos) {
  return static_cast<T>(sqlite3_column_int64(stmt, pos));
}

template <typename T>
T GetQueryResultImpl(sqlite3_stmt* stmt, int pos);

template <>
std::string GetQueryResultImpl<std::string>(sqlite3_stmt* stmt, int pos) {
  return reinterpret_cast<const char*>(sqlite3_column_text(stmt, pos));
}

} // namespace details


template <typename T>
T GetQueryResult(sqlite3_stmt* stmt, int pos) {
  return details::GetQueryResultImpl<T>(stmt, pos);
}

namespace details {

template<typename T> requires std::is_integral_v<T>
int OneBindStmtImpl(sqlite3_stmt* stmt, int pos, T value) {
  return sqlite3_bind_int64(stmt, pos, value);
}

int OneBindStmtImpl(sqlite3_stmt* stmt, int pos, const std::string& value) {
  return sqlite3_bind_text(stmt, pos, value.c_str(), value.size(), SQLITE_TRANSIENT);
}

int OneBindStmtImpl(sqlite3_stmt* stmt, int pos, const Timestamp& value) {
  return OneBindStmtImpl<int64_t>(stmt, pos,
      std::chrono::duration_cast<std::chrono::milliseconds>(value.time_since_epoch()).count());
}

} // namespace details

template<typename T>
void OneBindStmt(sqlite3* conn, sqlite3_stmt* stmt, int pos, const T& value) {
  CheckSqliteResult(conn, details::OneBindStmtImpl(stmt, pos, value));
}

} // namespace

namespace utils {

void CheckSqliteResult(sqlite3* conn, int res) {
  if (res != SQLITE_OK && res != SQLITE_DONE) {
    std::string errmsg{sqlite3_errmsg(conn)};
    const auto msg = fmt::format("Sqlite3 error: {}", errmsg);
    throw std::runtime_error(msg);
  }
}

void CreateTable(sqlite3* conn, const std::string_view& sql_create, const std::string_view& table_name) {
  char* errmsg = NULL;
  try {
    utils::CheckSqliteResult(conn, sqlite3_exec(conn, sql_create.data(), NULL, NULL, &errmsg));
    sqlite3_free(errmsg);
  } catch (...) {
    std::cout << fmt::format("Error during the table '{}' creation", table_name) << std::endl;
    sqlite3_free(errmsg);
    throw;
  }
}


SqliteStmtRunner::SqliteStmtRunner(sqlite3* conn, sqlite3_stmt* stmt) : conn_(conn), stmt_(stmt) {
}

SqliteStmtRunner::~SqliteStmtRunner() {
  CheckSqliteResult(conn_, sqlite3_reset(stmt_));
  CheckSqliteResult(conn_, sqlite3_clear_bindings(stmt_));
}

void SqliteStmtRunner::RunStmt() {
  for (int res = -1; res != SQLITE_DONE;) {
    while ((res = sqlite3_step(stmt_)) == SQLITE_ROW) {}
    CheckSqliteResult(conn_, res);
  }
  CheckSqliteResult(conn_, sqlite3_reset(stmt_));
  CheckSqliteResult(conn_, sqlite3_clear_bindings(stmt_));
}

} // namespace utils
