#pragma once

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <tables/sqlite3.h>
#include <fmt/format.h>

#include <types.hpp>

#include <iostream>

namespace utils {

void CheckSqliteResult(sqlite3*, int);

void CreateTable(sqlite3*, const std::string_view& sql_create, const std::string_view& table_name);

class SqliteStmtRunner {
 public:
  SqliteStmtRunner(sqlite3* conn, sqlite3_stmt* stmt);

  template <typename... Args>
  void BindStmt(const Args&... args);

  void RunStmt();

  template <typename... Args>
  std::vector<std::tuple<Args...>> RunStmtWithTupleResult();

  template <typename T>
  std::vector<T> RunStmtWithResult();

  ~SqliteStmtRunner();

 private:
  sqlite3* conn_;
  sqlite3_stmt* stmt_;
};

} // namespace utils


namespace utils {


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



template <typename... Args>
void SqliteStmtRunner::BindStmt(const Args&... args) {
  const std::tuple<Args...> tuple_args = std::tie(args...);
  [&]<std::size_t... I>(const std::index_sequence<I...>&){
    ([&](){
      OneBindStmt(conn_, stmt_, I + 1, std::get<I>(tuple_args));
    }(), ...);
  }(std::make_index_sequence<sizeof...(Args)>());
}

template <typename T>
std::vector<T> SqliteStmtRunner::RunStmtWithResult() {
  std::vector<T> result;
  for (int res = -1; res != SQLITE_DONE;) {
    while ((res = sqlite3_step(stmt_)) == SQLITE_ROW) {
      result.push_back(GetQueryResult<T>(stmt_, 0));
    }
    CheckSqliteResult(conn_, res);
  }
  CheckSqliteResult(conn_, sqlite3_reset(stmt_));
  CheckSqliteResult(conn_, sqlite3_clear_bindings(stmt_));
  return result;
}

template <typename... Args>
std::vector<std::tuple<Args...>> SqliteStmtRunner::RunStmtWithTupleResult() {
  std::vector<std::tuple<Args...>> result;
  for (int res = -1; res != SQLITE_DONE;) {
    while ((res = sqlite3_step(stmt_)) == SQLITE_ROW) {
      std::tuple<Args...> row;
      // compile-time loop on Args
      [&]<std::size_t... I>(const std::index_sequence<I...>&){
        ([&]() {
          std::get<I>(row) = GetQueryResult<typename std::tuple_element<I, decltype(row)>::type>(stmt_, I);
        }(), ...);
      }(std::make_index_sequence<sizeof...(Args)>());
      result.push_back(row);
    }
    CheckSqliteResult(conn_, res);
  }
  CheckSqliteResult(conn_, sqlite3_reset(stmt_));
  CheckSqliteResult(conn_, sqlite3_clear_bindings(stmt_));
  return result;
}

} // namespace utils
