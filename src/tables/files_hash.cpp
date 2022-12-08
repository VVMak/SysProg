#include <tables/files_hash.hpp>

#include <tables/utils.hpp>


constexpr char SQL_CREATE[] = \
"CREATE TABLE IF NOT EXISTS files_hash ("\
" id INTEGER PRIMARY KEY,"\
" hash INTEGER NOT NULL UNIQUE,"\
" FOREIGN KEY(id) REFERENCES files(id)"\
");"\
"CREATE INDEX IF NOT EXISTS hash_idx ON files_hash (hash);";

constexpr char SQL_INSERT[] = \
"INSERT INTO files_hash (id, hash) VALUES(?, ?);";

constexpr char SQL_FIND_BY_ID[] = \
"SELECT hash FROM files_hash\n"\
"WHERE id = ?;";

constexpr char SQL_FIND_BY_HASH[] = \
"SELECT id FROM files_hash\n"\
"WHERE hash = ?;";

constexpr char SQL_UPDATE[] = \
"UPDATE files_hash\n"\
"SET hash = ?\n"\
"WHERE id = ?;";

constexpr char SQL_DELETE[] = \
"DELETE FROM files_hash\n"\
"WHERE id = ?;";


FilesHashTable::FilesHashTable(sqlite3* conn) : conn_(conn) {
  utils::CreateTable(conn_, SQL_CREATE, "files_hash");
  try {
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_INSERT, -1, &insert_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_FIND_BY_HASH, -1, &find_by_hash_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_FIND_BY_ID, -1, &find_by_id_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_UPDATE, -1, &update_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_DELETE, -1, &delete_, NULL));
  } catch (...) {
    std::cout << "Error during the request preparation for table 'files_hash'" << std::endl;
    throw;
  }
}

void FilesHashTable::UpdateHash(FileId id, const Hash& hash) {
  utils::SqliteStmtRunner runner(conn_, update_);
  runner.BindStmt(hash, id);
  runner.RunStmt();
}
void FilesHashTable::Insert(FileId id, const Hash& hash)  {
  utils::SqliteStmtRunner runner(conn_, insert_);
  runner.BindStmt(id, hash);
  runner.RunStmt();
}
std::optional<Hash> FilesHashTable::GetHash(FileId id) const {
  utils::SqliteStmtRunner runner(conn_, find_by_id_);
  runner.BindStmt(id);
  auto result = runner.RunStmtWithResult<Hash>();
  return (result.empty() ? std::optional<Hash>{} : result[0]);
}
std::optional<FileId> FilesHashTable::FindByHash(const Hash& hash) const {
  utils::SqliteStmtRunner runner(conn_, find_by_hash_);
  runner.BindStmt(hash);
  auto result = runner.RunStmtWithResult<FileId>();
  return (result.empty() ? std::optional<FileId>{} : result[0]);
}
void FilesHashTable::Delete(FileId id) {
  utils::SqliteStmtRunner runner(conn_, find_by_id_);
  runner.BindStmt(id);
  runner.RunStmt();
}

FilesHashTable::~FilesHashTable() {
  sqlite3_finalize(insert_);
  sqlite3_finalize(find_by_hash_);
  sqlite3_finalize(find_by_id_);
  sqlite3_finalize(update_);
  sqlite3_finalize(delete_);
}
