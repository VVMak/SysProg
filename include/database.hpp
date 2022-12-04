#include <file_list.hpp>
#include <tables/actions.hpp>
#include <tables/files_content.hpp>
#include <tables/files_hash.hpp>
#include <tables/files.hpp>

#include <file_fwd.hpp>
#include <process_fwd.hpp>


class Database : public std::enable_shared_from_this<Database> {
 public:
  Database(const std::string& path);
  ~Database();

  std::shared_ptr<FileList> GetBlackList();
  std::shared_ptr<FileList> GetWhiteList();

  std::vector<File> FindActions(Process&, const Timestamp& t);
  std::unordered_map<std::string, std::size_t> FindActionsOnDirs(Process&, const Timestamp&);
  void AddAction(Process&, File& target);
  void DeleteActions(Process&);

  std::optional<Path> GetFilePath(File&);
  File GetFileByPath(const Path& path);

  std::optional<std::string> GetContent(File&);
  bool FileWasSaved(File&);
  void SaveFileContent(File&);

  void SaveFileHash(File&);
  std::optional<Hash> GetFileHash(File&);

  std::optional<File> FindFileByHash(const Hash& hash);

  void DeleteOld(const Timestamp&);

 private:
  sqlite3* conn_;

  std::unique_ptr<ActionsTable> actions_;
  std::unique_ptr<FilesContentTable> files_content_;
  std::unique_ptr<FilesHashTable> files_hash_;
  std::unique_ptr<FilesTable> files_;
  std::shared_ptr<FileList> blacklist_;
  std::shared_ptr<FileList> whitelist_;
};
