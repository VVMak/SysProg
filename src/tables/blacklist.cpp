#include <tables/blacklist.hpp>


BlackListTable::BlackListTable(sqlite3* conn) : ListTable(conn) {
  constexpr auto TABLE_NAME = "blacklist";
  utils::CreateTable(conn, fmt::format(SQL_CREATE, TABLE_NAME), TABLE_NAME);
  try {
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, fmt::format(SQL_INSERT, TABLE_NAME).data(), -1, &insert_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, fmt::format(SQL_FIND, TABLE_NAME).data(), -1, &find_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, fmt::format(SQL_LIST, TABLE_NAME).data(), -1, &list_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, fmt::format(SQL_DELETE, TABLE_NAME).data(), -1, &delete_, NULL));
  } catch (...) {
    std::cout << fmt::format("Error during the request preparation for table '{}'", TABLE_NAME) << std::endl;
    throw;
  }
}

BlackListTable::~BlackListTable() {}
