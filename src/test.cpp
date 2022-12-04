#include <iostream>
#include <string>

#include <fmt/format.h>

#include <tables/files.hpp>
#include <tables/actions.hpp>


using namespace std::string_literals;
using namespace std::chrono_literals;

int main() {
  sqlite3* conn;
  auto res = sqlite3_open("test.db", &conn);
  if (res != SQLITE_OK) {
    sqlite3_close(conn);
    throw std::runtime_error("Can't connect to db, error: "s + sqlite3_errmsg(conn));
  }
  try {
    FilesTable files(conn);
    std::cout << files.Insert("/angry/virus") << std::endl;
    std::cout << files.Insert("/path/to/file") << std::endl;
    std::cout << files.Insert("/another/path/to/file") << std::endl;
    ActionsTable actions(conn);
  } catch (const std::runtime_error& e) {
    std::cout << fmt::format("Error during the calculations:\n{}", e.what()) << std::endl;
  }
  sqlite3_close(conn);
  return 0;
}