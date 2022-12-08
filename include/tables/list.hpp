#pragma once

#include <vector>

#include <tables/utils.hpp>
#include <types.hpp>


class ListTable {
 public:
  ListTable(sqlite3*);
  virtual void Insert(FileId);
  virtual std::vector<FileId> List() const;
  virtual bool Find(FileId) const;
  virtual void Delete(FileId);

  virtual ~ListTable();
 protected:
  sqlite3* conn_;

  sqlite3_stmt* insert_ = NULL;
  sqlite3_stmt* list_ = NULL;
  sqlite3_stmt* find_ = NULL;
  sqlite3_stmt* delete_ = NULL;
};

namespace {

constexpr char SQL_CREATE[] = \
"CREATE TABLE IF NOT EXISTS {} ("\
" id INTEGER PRIMARY KEY,"\
" FOREIGN KEY(id) REFERENCES files(id)"\
");";

constexpr char SQL_INSERT[] = \
"INSERT INTO {} (id) VALUES(?);";

constexpr char SQL_FIND[] = \
"SELECT count(*) FROM {}\n"\
"WHERE id = ?;";

constexpr char SQL_LIST[] = \
"SELECT id FROM {};";

constexpr char SQL_DELETE[] = \
"DELETE FROM {}\n"\
"WHERE id = ?;";

} // namespace

