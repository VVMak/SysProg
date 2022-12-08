#include <tables/files_content.hpp>

#include <tables/utils.hpp>


constexpr char SQL_CREATE[] = \
"CREATE TABLE IF NOT EXISTS files_content ("\
" id INTEGER PRIMARY KEY,"\
" content TEXT NOT NULL,"\
" FOREIGN KEY(id) REFERENCES files(id)"\
");";

constexpr char SQL_INSERT[] = \
"INSERT INTO files_content (id, content) VALUES(?, ?);";

constexpr char SQL_SELECT[] = \
"SELECT content FROM files_content\n"\
"WHERE id = ?;";

constexpr char SQL_COUNT[] = \
"SELECT count(*) FROM files_content\n"\
"WHERE id = ?;";

constexpr char SQL_UPDATE[] = \
"UPDATE files_content\n"\
"SET content = ?\n"\
"WHERE id = ?;";

constexpr char SQL_DELETE[] = \
"DELETE FROM files_content\n"\
"WHERE id = ?;";


FilesContentTable::FilesContentTable(sqlite3* conn) : conn_(conn) {
  utils::CreateTable(conn_, SQL_CREATE, "files_content");
  try {
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_INSERT, -1, &insert_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_SELECT, -1, &select_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_COUNT, -1, &count_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_UPDATE, -1, &update_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_DELETE, -1, &delete_, NULL));
  } catch (...) {
    std::cout << "Error during the request preparation for table 'files_content'" << std::endl;
    throw;
  }
}

void FilesContentTable::UpdateContent(FileId id, const std::string& content) {
  utils::SqliteStmtRunner runner(conn_, update_);
  runner.BindStmt(content, id);
  runner.RunStmt();
}
void FilesContentTable::Insert(FileId id, const std::string& content) {
  utils::SqliteStmtRunner runner(conn_, insert_);
  runner.BindStmt(id, content);
  runner.RunStmt();
}
bool FilesContentTable::ContentWasSaved(FileId id) const {
  utils::SqliteStmtRunner runner(conn_, count_);
  runner.BindStmt(id);
  return runner.RunStmtWithResult<std::size_t>().at(0) > 0;
}
std::optional<std::string> FilesContentTable::GetContent(FileId id) const {
  utils::SqliteStmtRunner runner(conn_, count_);
  runner.BindStmt(id);
  auto result = runner.RunStmtWithResult<std::string>();
  return (result.empty() ? std::optional<std::string>() : result[0]);
}
void FilesContentTable::Delete(FileId id) {
  utils::SqliteStmtRunner runner(conn_, delete_);
  runner.BindStmt(id);
  runner.RunStmt();
}

FilesContentTable::~FilesContentTable() {
  sqlite3_finalize(insert_);
  sqlite3_finalize(select_);
  sqlite3_finalize(count_);
  sqlite3_finalize(update_);
  sqlite3_finalize(delete_);
}
