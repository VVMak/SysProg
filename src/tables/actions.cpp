#include <tables/actions.hpp>


#include <tables/utils.hpp>

constexpr char SQL_CREATE[] = \
"DROP TABLE IF EXISTS actions;"\
"CREATE TABLE IF NOT EXISTS actions ("\
" id INTEGER PRIMARY KEY,"\
" bin INTEGER NOT NULL,"\
" target INTEGER NOT NULL,"\
" target_dir INTEGER NOT NULL,"\
" timepoint INTEGER NOT NULL,"\
" FOREIGN KEY(bin) REFERENCES files(id),"\
" FOREIGN KEY(target) REFERENCES files(id)"\
");"\
"CREATE INDEX IF NOT EXISTS bin_idx ON actions (bin);"\
"CREATE INDEX IF NOT EXISTS target_dir_idx ON actions (target_dir);"\
"CREATE INDEX IF NOT EXISTS timepoint_idx ON actions (timepoint);";

constexpr char SQL_INSERT[] = \
"INSERT INTO actions (bin, target, target_dir, timepoint) VALUES(?, ?, ?, ?);";

constexpr char SQL_SELECT[] = \
"SELECT target FROM actions\n"\
"WHERE bin = ? AND timepoint >= ?;";

constexpr char SQL_SELECT_OLD[] = \
"SELECT target FROM actions\n"\
"WHERE timepoint < ?;";

constexpr char SQL_SELECT_WITH_DIRS[] = \
"SELECT target_dir, target FROM actions\n"\
"WHERE bin = ? AND timepoint >= ?;";

constexpr char SQL_DELETE_BY_BIN[] = \
"DELETE FROM actions\n"\
"WHERE bin = ?;";

constexpr char SQL_DELETE_BY_TIME[] = \
"DELETE FROM actions\n"\
"WHERE timepoint < ?;";

constexpr char SQL_DELETE_ALL[] = \
"DELETE FROM actions;";


ActionsTable::ActionsTable(sqlite3* conn) : conn_(conn) {
  utils::CreateTable(conn, SQL_CREATE, "actions");
  try {
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_INSERT, -1, &insert_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_SELECT, -1, &select_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_SELECT_OLD, -1, &select_old_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_SELECT_WITH_DIRS, -1, &select_with_dirs_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_DELETE_BY_BIN, -1, &delete_by_bin_, NULL));
    utils::CheckSqliteResult(conn_, sqlite3_prepare_v2(conn_, SQL_DELETE_BY_TIME, -1, &delete_by_time_, NULL));
  } catch (...) {
    std::cout << "Error during the request preparation for table 'actions'" << std::endl;
    throw;
  }
}

void ActionsTable::AddAction(FileId bin, FileId file, FileId dir, const Timestamp& t) {
  utils::SqliteStmtRunner runner(conn_, insert_);
  runner.BindStmt(bin, file, dir, t);
  runner.RunStmt();
}

std::vector<FileId> ActionsTable::FindActions(FileId bin, const Timestamp& t) const {
  utils::SqliteStmtRunner runner(conn_, select_);
  runner.BindStmt(bin, t);
  return runner.RunStmtWithResult<FileId>();
}
std::unordered_map<FileId, std::vector<FileId>> ActionsTable::FindActionsOnDirs(FileId bin, const Timestamp& t) const {
  utils::SqliteStmtRunner runner(conn_, select_with_dirs_);
  runner.BindStmt(bin, t);
  auto query_result = runner.RunStmtWithTupleResult<FileId, FileId>();
  std::unordered_map<FileId, std::vector<FileId>> result;
  for (auto [dir, file] : query_result) {
    result[dir].push_back(file);
  }
  return result;
}
void ActionsTable::DeleteActions(FileId bin) {
  utils::SqliteStmtRunner runner(conn_, delete_by_bin_);
  runner.BindStmt(bin);
  runner.RunStmt();
}
std::vector<FileId> ActionsTable::DeleteOld(const Timestamp& t) {
  utils::SqliteStmtRunner select_runner(conn_, select_old_);
  select_runner.BindStmt(t);
  auto deleted = select_runner.RunStmtWithResult<FileId>();
  utils::SqliteStmtRunner del_runner(conn_, delete_by_time_);
  del_runner.BindStmt(t);
  del_runner.RunStmt();
  return deleted;
}

ActionsTable::~ActionsTable() {
  sqlite3_finalize(insert_);
  sqlite3_finalize(select_);
  sqlite3_finalize(select_old_);
  sqlite3_finalize(select_with_dirs_);
  sqlite3_finalize(delete_by_bin_);
  sqlite3_finalize(delete_by_time_);
}
