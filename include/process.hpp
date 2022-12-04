#include <sys/types.h>

#include <file_fwd.hpp>
#include <database_fwd.hpp>

class Process {
 public:
  Process(unsigned int pid, DatabasePtr db);
  File GetBinary();
  bool HasWriteAccess();
  bool HasAccessToDb();
  void Ban();
  void Kill();
 private:
  unsigned int pid_;
  DatabasePtr db_;
};
