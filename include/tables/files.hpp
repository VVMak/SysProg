#pragma once

#include <optional>

#include "sqlite3.h"
#include <types.hpp>

class FilesTable {
 public:
  FilesTable(sqlite3*);

  FileId Insert(const Path&);
  std::optional<Path> FindPath(FileId) const;
  std::optional<FileId> FindFileId(const Path&) const;
  void Delete(FileId);

  ~FilesTable();
 private:
  sqlite3* conn_;
  
  sqlite3_stmt* insert_;
  sqlite3_stmt* select_by_id_;
  sqlite3_stmt* select_by_path_;
  sqlite3_stmt* delete_;
};
