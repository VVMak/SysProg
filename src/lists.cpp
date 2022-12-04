#include <boost/program_options.hpp>
#include <fmt/format.h>

#include <database.hpp>


namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  po::options_description desc("Options");
  desc.add_options()
    ("help,h", "Show help")
    ("list,L", po::value<std::string>(), 
        "Choose 'black' or 'white' (also possible 'b' or 'w') (without quotes). "\
        "If you won't add other params, then it will print corresponding list.")
    ("add,A", po::value<std::vector<std::string>>(),
        "The names of the files that will be ADDED to the corresponding list. "\
        "Adding in one list will automatically remove it from another.")
    ("remove,R", po::value<std::vector<std::string>>(),
        "The names of the files that will be REMOVED from the corresponding list. "\
        "Deletion from one list WON'T add it in another.")
  ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  if (vm.count("help") || !vm.count("list")) {
    std::cout << desc << std::endl;
    return 0;
  }
  auto db = std::make_shared<Database>("test.db");
  std::shared_ptr<FileList> list, opposite_list;
  auto list_type = vm["list"].as<std::string>();
  if (list_type == "black" || list_type == "b") {
    list = db->GetBlackList();
    opposite_list = db->GetWhiteList();
  } else if (list_type == "white" || list_type == "w") {
    list = db->GetWhiteList();
    opposite_list = db->GetBlackList();
  } else {
    std::cout << fmt::format("Unknown list type '{}', watch help (--help or -h).", list_type) << std::endl;
    return 0;
  }
  if (!vm.count("add") && !vm.count("remove")) {
    auto files = list->ListFiles();
    for (auto file : files) {
      std::cout << file.string() << std::endl;
    }
    return 0;
  }
  if (vm.count("add")) {
    auto add_list = vm["add"].as<std::vector<std::string>>();
    for (auto path : add_list) {
      auto f = db->GetFileByPath(path);
      list->Add(f);
      opposite_list->Delete(f);
    }
  }
  if (vm.count("remove")) {
    auto remove_list = vm["remove"].as<std::vector<std::string>>();
    for (auto path : remove_list) {
      list->Delete(db->GetFileByPath(path));
    }
  }
  return 0;
}