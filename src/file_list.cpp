#include <file_list.hpp>

#include <file.hpp>

FileList::FileList(std::unique_ptr<ListTable>&& list_table, DatabasePtr db)
    : table_(std::move(list_table)), db_(db) {}

void FileList::Add(File file) {
  if (table_->Find(file.GetFileId())) {
    return;
  }
  table_->Insert(file.GetFileId());
}

std::vector<Path> FileList::ListFiles() const {
  auto list = table_->List();
  std::vector<Path> result;
  result.reserve(list.size());
  for (auto id : list) {
    result.push_back(File(id, db_).GetPath());
  }
  return result;
}

bool FileList::Find(File file) const {
  return table_->Find(file.GetFileId());
}

void FileList::Delete(File file) {
  table_->Delete(file.GetFileId());
}
