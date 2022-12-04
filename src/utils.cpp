#include <utils.hpp>

#include <database.hpp>
#include <file.hpp>
#include <process.hpp>

#include <boost/crc.hpp>

namespace utils {

Hash CalculateHash(const std::string& str) {
  boost::crc_32_type hasher;
  hasher.process_bytes(str.data(), str.size());
  return hasher.checksum();
}

void BanProcessAndRestoreFiles(DatabasePtr db, Process& p) {
  p.Ban();
  p.Kill();
  auto corrupted = db->FindActions(p, std::chrono::steady_clock::time_point{});
  db->DeleteActions(p);
  for (auto file : corrupted) {
    file.RestoreContent();
  }
}

} // namespace utils
