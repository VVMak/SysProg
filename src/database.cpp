#include <database.hpp>

#include <iostream>
#include <string>

#include <file.hpp>
#include <process.hpp>
#include <utils.hpp>
#include <tables/blacklist.hpp>
#include <tables/whitelist.hpp>

namespace {

using namespace std::string_literals;

sqlite3* CreateConnection(const std::string& path) {
  sqlite3* conn;
  auto res = sqlite3_open(path.c_str(), &conn);
  if (res != SQLITE_OK) {
    sqlite3_close(conn);
    throw std::runtime_error("Can't connect to db, error: "s + sqlite3_errmsg(conn));
  }
  return conn;
}

} // namespace

Database::Database(const std::string& path)
    : conn_(CreateConnection(path))
    , actions_(std::make_unique<ActionsTable>(conn_))
    , files_content_(std::make_unique<FilesContentTable>(conn_))
    , files_hash_(std::make_unique<FilesHashTable>(conn_))
    , blacklist_(std::make_shared<FileList>(std::make_unique<BlackListTable>(conn_), shared_from_this()))
    , whitelist_(std::make_shared<FileList>(std::make_unique<WhiteListTable>(conn_), shared_from_this())) {
}

Database::~Database() {
  if (conn_ != NULL) {
    int res = sqlite3_close(conn_);
    std::cout << "DB was closed with result " + std::to_string(res) << std::endl; 
  }
}

std::shared_ptr<FileList> Database::GetBlackList() {
  return blacklist_;
}

std::shared_ptr<FileList> Database::GetWhiteList() {
  return whitelist_;
}

std::vector<File> Database::FindActions(Process& p, const Timestamp& t) {
  auto vec_id = actions_->FindActions(p.GetBinary().GetFileId(), t);
  std::vector<File> result;
  result.reserve(vec_id.size());
  for (auto id : vec_id) {
    result.emplace_back(id, shared_from_this());
  }
  return result;
}
std::unordered_map<std::string, std::size_t> Database::FindActionsOnDirs(Process& p, const Timestamp& t) {
  auto dir_to_targets = actions_->FindActionsOnDirs(p.GetBinary().GetFileId(), t);
  std::unordered_map<std::string, std::size_t> result/* {{"/", {}}} */;
  for (auto [dir, file_ids] : dir_to_targets) {
    const auto dir_path = files_->FindPath(dir).value().string();
    result.insert({dir_path, file_ids.size()});
  }
  return result;
}

void Database::AddAction(Process& p, File& f) {
  actions_->AddAction(p.GetBinary().GetFileId(), f.GetFileId(),
      GetFileByPath(f.GetPath().parent_path()).GetFileId(),
      std::chrono::steady_clock::now());
}
void Database::DeleteActions(Process& p) {
  actions_->DeleteActions(p.GetBinary().GetFileId());
}

std::optional<Path> Database::GetFilePath(File& f) {
  return files_->FindPath(f.GetFileId());
}
File Database::GetFileByPath(const Path& path) {
  auto id_opt = files_->FindFileId(path);
  if (id_opt) {
    return File(*id_opt, shared_from_this());
  }
  return File(files_->Insert(path), shared_from_this());
}

std::optional<std::string> Database::GetContent(File& f) {
  return files_content_->GetContent(f.GetFileId());
}
bool Database::FileWasSaved(File& f) {
  return files_content_->ContentWasSaved(f.GetFileId());
}
void Database::SaveFileContent(File& f) {
  auto content = f.ReadContent();
  if (FileWasSaved(f)) {
    files_content_->UpdateContent(f.GetFileId(), std::move(content));
  } else {
    files_content_->Insert(f.GetFileId(), std::move(content));
  }
}

void Database::SaveFileHash(File& f) {
  auto hash = utils::CalculateHash(f.ReadContent());
  if (GetFileHash(f)) {
    files_hash_->UpdateHash(f.GetFileId(), hash);
  } else {
    files_hash_->Insert(f.GetFileId(), hash);
  }
}
std::optional<Hash> Database::GetFileHash(File& f) {
  return files_hash_->GetHash(f.GetFileId());
}

std::optional<File> Database::FindFileByHash(const Hash& hash) {
  auto id_opt = files_hash_->FindByHash(hash);
  return (id_opt ? File(*id_opt, shared_from_this()) : std::optional<File>{});
}

void Database::DeleteOld(const Timestamp& t) {
  auto deleted = actions_->DeleteOld(t);
  for (auto id : deleted) {
    if (!(actions_->FindActions(id, t).empty())) { continue; }
    if (whitelist_->table_->Find(id)) { continue; }
    if (blacklist_->table_->Find(id)) { continue; }

    files_content_->Delete(id);
    files_hash_->Delete(id);
    files_->Delete(id);
  }
}
