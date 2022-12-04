#include <tables/list.hpp>

class WhiteListTable : public ListTable {
 public:
  WhiteListTable(sqlite3*);
  ~WhiteListTable() override;
};
