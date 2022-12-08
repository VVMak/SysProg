#include <tables/list.hpp>

class BlackListTable : public ListTable {
 public:
  BlackListTable(sqlite3*);
  ~BlackListTable() override;
};
