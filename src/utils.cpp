#include <utils.hpp>

#include <fstream>
#include <syslog.h>

#include <boost/crc.hpp>
#include <fmt/format.h>

#include <database.hpp>
#include <file.hpp>
#include <process.hpp>

#include <settings.hpp>

namespace utils {

Hash CalculateHash(const std::string& str) {
  boost::crc_32_type hasher;
  hasher.process_bytes(str.data(), str.size());
  return hasher.checksum();
}

void BanProcessAndRestoreFiles(DatabasePtr db, Process& p) {
  syslog(LOG_ERR, "%s", fmt::format("Detect process {} with binary {}", p.GetPid(), p.GetBinary().GetPath().string()).c_str());
  p.Ban();
  p.Kill();
  auto corrupted = db->FindActions(p, std::chrono::steady_clock::time_point{});
  db->DeleteActions(p);
  for (auto file : corrupted) {
    file.RestoreContent();
  }
}

std::filesystem::path GetFilePathByFd(int fd) {
  return std::filesystem::read_symlink("/proc/self/fd/" + std::to_string(fd));
}

void CreateDefaultFiles() {
  try {
    std::filesystem::create_directory(settings::PROJECT_FILES_PATH);
    if (!std::filesystem::exists(settings::DEFAULT_CONFIG_PATH)) {
      std::ofstream config_file(settings::DEFAULT_CONFIG_PATH);
      config_file.close();
    }
  } catch (const std::runtime_error& e) {
    syslog(LOG_WARNING, "%s", fmt::format(
        "Can't create project dir in {} or config file in this folder",
        settings::PROJECT_FILES_PATH
    ).c_str());
  }
}

} // namespace utils
