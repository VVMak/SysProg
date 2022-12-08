#include <sys/types.h>

#include <file_fwd.hpp>
#include <database_fwd.hpp>
#include <settings.hpp>

class Process {
 public:
  Process(unsigned int pid, DatabasePtr db);
  pid_t GetPid() const;
  File GetBinary();
  bool HasWriteAccess(const settings::Config&);
  bool HasAccessToDb();
  void Ban();
  void Kill();
 private:
  unsigned int pid_;
  DatabasePtr db_;
};
