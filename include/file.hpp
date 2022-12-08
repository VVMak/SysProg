#pragma once

#include <database_fwd.hpp>
#include <types.hpp>

class File {
 public:
  File(const Path&, const DatabasePtr);

  Path GetPath();

  void SaveContent();
  void RestoreContent();

  Hash GetHash();

  bool IsAntivirusDb();

  friend class Database;
  friend class FileList;

  File(FileId id, const DatabasePtr db);
 private:
  FileId GetFileId();
  
  std::string GetContent();

  std::string ReadContent();

  FileId id_;
  DatabasePtr db_;
};
