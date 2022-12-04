#include <file.hpp>

#include <database.hpp>
#include <utils.hpp>

#include <fstream>

File::File(const Path& path, const DatabasePtr db) : File(db->GetFileByPath(path)) {}

File::File(FileId id, const DatabasePtr db) : id_(id), db_(db) {}

FileId File::GetFileId() {
  return id_;
}

Path File::GetPath() {
  return *(db_->GetFilePath(*this));
}

std::string File::ReadContent() {
  std::ifstream fin(GetPath());
  std::stringstream content;
  content << fin.rdbuf();
  return content.str();
}

std::string File::GetContent() {
  const auto db_file_content_opt = db_->GetContent(*this);
  if (db_file_content_opt.has_value()) {
    return *db_file_content_opt;
  } 
  return ReadContent();
}

void File::SaveContent() {
  db_->SaveFileContent(*this);
}

void File::RestoreContent() {
  std::ofstream fout(GetPath());
  fout << (GetContent());
}

Hash File::GetHash() {
  auto hash_opt = db_->GetFileHash(*this);
  if (hash_opt.has_value()) {
    return *hash_opt;
  }
  return utils::CalculateHash(GetContent());
}

bool File::IsAntivirusDb() {
  return id_ == 0;
}
