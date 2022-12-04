#include <types.hpp>
#include "sqlite3.h"

#include <unordered_map>
#include <vector>

class ActionsTable {
 public:
  ActionsTable(sqlite3*);

  void AddAction(FileId bin, FileId file, FileId dir, const Timestamp&);
  std::vector<FileId> FindActions(FileId bin, const Timestamp&) const;
  std::unordered_map<FileId, std::vector<FileId>> FindActionsOnDirs(FileId bin, const Timestamp&) const;
  void DeleteActions(FileId bin);
  std::vector<FileId> DeleteOld(const Timestamp&);

  ~ActionsTable();
 private:
  sqlite3* conn_;

  sqlite3_stmt* insert_;
  sqlite3_stmt* select_;
  sqlite3_stmt* select_old_;
  sqlite3_stmt* select_with_dirs_;
  sqlite3_stmt* delete_by_bin_;
  sqlite3_stmt* delete_by_time_;
};
