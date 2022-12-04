#include <process.hpp>

#include <filesystem>
#include <iostream>

#include <database.hpp>
#include <file.hpp>

Process::Process(unsigned int pid, DatabasePtr db) : pid_(pid), db_(db) {}

File Process::GetBinary() {
  auto file_link = std::filesystem::path("/pid/" + std::to_string(pid_) + "/exe");
  auto path = std::filesystem::read_symlink(file_link);
  return db_->GetFileByPath(path);
}

bool Process::HasWriteAccess() {
  const auto bin_hash = GetBinary().GetHash();
  const auto file_opt = db_->FindFileByHash(bin_hash);
  if (!file_opt) { return false; }
  if (db_->GetWhiteList()->Find(*file_opt)) {
    return true;
  }
  if (db_->GetBlackList()->Find(*file_opt)) {
    return false;
  }
  auto t = std::chrono::steady_clock::now() - std::chrono::milliseconds(100);
  db_->DeleteOld(t);
  auto dirs_to_files = db_->FindActionsOnDirs(*this, t);
  for (auto [dir, num] : dirs_to_files) {
    if (num >= 3) {
      return false;
    }
  }
  return true;
}

bool Process::HasAccessToDb() {
  return GetBinary().GetHash() == 123456789 /* HARDCODED HASH */;
}

void Process::Ban() {
  if (db_->GetWhiteList()->Find(GetBinary())) {
    throw std::runtime_error("Can't ban process in White List");
  } 
  db_->GetBlackList()->Add(GetBinary());
}

void Process::Kill() {
  if (db_->GetWhiteList()->Find(GetBinary())) {
    throw std::runtime_error("Can't kill process in White List");
  }
  std::cout << "I will kill process with pid " << pid_ << " and path " << GetBinary().GetPath() << std::endl;
}
