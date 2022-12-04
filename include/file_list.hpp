#pragma once

#include <vector>

#include <database_fwd.hpp>
#include <file_fwd.hpp>

#include <tables/list.hpp>

class FileList {
 public:
  FileList(std::unique_ptr<ListTable>&& list_table, DatabasePtr db);
  void Add(File);
  std::vector<Path> ListFiles() const;
  bool Find(File) const;
  void Delete(File);

  friend class Database;

 private:
  std::unique_ptr<ListTable> table_;
  DatabasePtr db_;
};
