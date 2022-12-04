#include <types.hpp>

#include <database_fwd.hpp>
#include <process_fwd.hpp>

namespace utils {

Hash CalculateHash(const std::string&);

void BanProcessAndRestoreFiles(DatabasePtr db, Process& p);

} // namespace utils
