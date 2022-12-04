#include <optional>

#include <tables/sqlite3.h>

#include <types.hpp>

class FilesHashTable {
 public:
  FilesHashTable(sqlite3*);

  void UpdateHash(FileId, const Hash&);
  void Insert(FileId, const Hash&);
  std::optional<Hash> GetHash(FileId) const;
  std::optional<FileId> FindByHash(const Hash&) const;
  void Delete(FileId);

  ~FilesHashTable();
 private:
  sqlite3* conn_;
  
  sqlite3_stmt* insert_;
  sqlite3_stmt* find_by_hash_;
  sqlite3_stmt* find_by_id_;
  sqlite3_stmt* update_;
  sqlite3_stmt* delete_;
};
