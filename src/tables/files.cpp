#include <tables/files.hpp>

#include <iostream>
#include <string_view>

#include <tables/utils.hpp>

constexpr char SQL_CREATE[] = \
"CREATE TABLE IF NOT EXISTS files ("\
" id INTEGER PRIMARY KEY,"\
" path TEXT NOT NULL UNIQUE"\
");"\
"CREATE INDEX IF NOT EXISTS path_idx ON files (path);";

constexpr char SQL_INSERT[] = \
"INSERT INTO files (path) VALUES(?);";

constexpr char SQL_SELECT_BY_ID[] = \
"SELECT path FROM files\n"\
"WHERE id = ?;";

constexpr char SQL_SELECT_BY_PATH[] = \
"SELECT id FROM files\n"\
"WHERE path = ?;";

constexpr char SQL_DELETE[] = \
"DELETE FROM files\n"\
"WHERE id = ?;";

using namespace std::string_view_literals;

FilesTable::FilesTable(sqlite3* conn) : conn_(conn) {
  utils::CreateTable(conn, SQL_CREATE, "files");
  try {
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_INSERT, -1, &insert_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_SELECT_BY_ID, -1, &select_by_id_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_SELECT_BY_PATH, -1, &select_by_path_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_DELETE, -1, &delete_, NULL));
  } catch (...) {
    std::cout << "Error during the request preparation for table 'files'" << std::endl;
    throw;
  }
}

FilesTable::~FilesTable() {
  sqlite3_finalize(insert_);
  sqlite3_finalize(select_by_id_);
  sqlite3_finalize(select_by_path_);
  sqlite3_finalize(delete_);
}

FileId FilesTable::Insert(const Path& path) {
  utils::SqliteStmtRunner runner(conn_, insert_);
  runner.BindStmt(path.string());
  runner.RunStmt();
  return sqlite3_last_insert_rowid(conn_);
}

std::optional<Path> FilesTable::FindPath(FileId id) const {
  utils::SqliteStmtRunner runner(conn_, select_by_id_);
  runner.BindStmt<FileId>(id);
  const auto result = runner.RunStmtWithResult<std::string>();
  return (result.empty() ? std::optional<Path>{} : result[0]);
}

std::optional<FileId> FilesTable::FindFileId(const Path& path) const {
  utils::SqliteStmtRunner runner(conn_, select_by_path_);
  runner.BindStmt<std::string>(path.string());
  const auto result = runner.RunStmtWithResult<FileId>();
  return (result.empty() ? std::optional<FileId>{} : result[0]);
}

void FilesTable::Delete(FileId id)
{
  utils::SqliteStmtRunner runner(conn_, delete_);
  runner.BindStmt<FileId>(id);
  runner.RunStmt();
}
