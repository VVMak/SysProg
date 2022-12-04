#include <tables/list.hpp>

#include <tables/utils.hpp>


ListTable::ListTable(sqlite3* conn) : conn_(conn) {}

void ListTable::Insert(FileId file) {
  utils::SqliteStmtRunner runner(conn_, insert_);
  runner.BindStmt(file);
  runner.RunStmt();
}
std::vector<FileId> ListTable::List() const {
  utils::SqliteStmtRunner runner(conn_, list_);
  return runner.RunStmtWithResult<FileId>();
}
bool ListTable::Find(FileId file) const {
  utils::SqliteStmtRunner runner(conn_, find_);
  runner.BindStmt(file);
  return runner.RunStmtWithResult<std::size_t>().at(0) > 0;
}
void ListTable::Delete(FileId file) {
  utils::SqliteStmtRunner runner(conn_, delete_);
  runner.BindStmt(file);
  runner.RunStmt();
}

ListTable::~ListTable() {
  sqlite3_finalize(insert_);  
  sqlite3_finalize(list_);
  sqlite3_finalize(find_);
  sqlite3_finalize(delete_);
}
