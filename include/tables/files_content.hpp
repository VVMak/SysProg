#include <optional>

#include <tables/sqlite3.h>

#include <types.hpp>

class FilesContentTable {
 public:
  FilesContentTable(sqlite3*);

  void UpdateContent(FileId, const std::string&);
  void Insert(FileId, const std::string&);
  bool ContentWasSaved(FileId) const;
  std::optional<std::string> GetContent(FileId) const;
  void Delete(FileId);

  ~FilesContentTable();
 private:
  sqlite3* conn_;
  
  sqlite3_stmt* insert_;
  sqlite3_stmt* select_;
  sqlite3_stmt* count_;
  sqlite3_stmt* update_;
  sqlite3_stmt* delete_;
};
