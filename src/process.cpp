#include <process.hpp>

#include <filesystem>
#include <iostream>

#include <fmt/format.h>

#include <database.hpp>
#include <file.hpp>
#include <settings.hpp>
#include <syslog.h>

Process::Process(unsigned int pid, DatabasePtr db) : pid_(pid), db_(db) {
  GetBinary(); // Save binary in db
}

pid_t Process::GetPid() const {
  return pid_;
}

File Process::GetBinary() {
  std::filesystem::path file_link; 
  try {
    file_link = fmt::format("/proc/{}/exe", pid_);
  } catch (const std::filesystem::filesystem_error& e) {
    syslog(LOG_WARNING, "Can't find binary file of process %d, error:\n %s", pid_, e.what());
    throw std::runtime_error("Process has already finished");
  }
  auto path = std::filesystem::read_symlink(file_link);
  return db_->GetFileByPath(path);
}

bool Process::HasWriteAccess(const settings::Config& config) {
  const auto bin_hash = GetBinary().GetHash();
  const auto file_opt = db_->FindFileByHash(bin_hash);
  if (!file_opt) { return false; }
  if (db_->GetWhiteList()->Find(*file_opt)) {
    return true;
  }
  if (db_->GetBlackList()->Find(*file_opt)) {
    return false;
  }
  auto t = std::chrono::steady_clock::now() - config.GetCheckTime();
  db_->DeleteOld(t);
  auto dirs_to_files = db_->FindActionsOnDirs(*this, t);
  for (auto [dir, num] : dirs_to_files) {
    if (num >= config.GetMaxActions()) {
      return false;
    }
  }
  return true;
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
