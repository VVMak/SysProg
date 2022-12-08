#include <settings.hpp>

#include <iostream>
#include <optional>
#include <syslog.h>

namespace settings {

namespace {

using namespace std::literals::chrono_literals;

constexpr auto DB_PATH_CONFIG_NAME = "db_path";
const auto DEFAULT_DB_PATH = fmt::format("{}/base.db", PROJECT_FILES_PATH);
constexpr auto CHECK_TIME_CONFIG_NAME = "check_time";
constexpr auto DEFAULT_CHECK_TIME = 1000ms;
constexpr auto MAX_ACTIONS_CONFIG_NAME = "max_actions";
constexpr auto DEFAULT_MAX_ACTIONS = 4;

} // namespace

namespace po = boost::program_options;

Config::Config(const std::string& path) : desc_("Config") {
  desc_.add_options()
    (DB_PATH_CONFIG_NAME, po::value<std::string>()->default_value(DEFAULT_DB_PATH))
    (CHECK_TIME_CONFIG_NAME, po::value<uint64_t>()->default_value(DEFAULT_CHECK_TIME.count())) // in milliseconds
    (MAX_ACTIONS_CONFIG_NAME, po::value<std::size_t>()->default_value(DEFAULT_MAX_ACTIONS))
  ;
  po::variables_map vm;
  try {
    po::store(po::parse_config_file(path.data(), desc_), vm);
    db_path_ = vm[DB_PATH_CONFIG_NAME].as<std::string>();
    check_time_ = std::chrono::milliseconds(vm[CHECK_TIME_CONFIG_NAME].as<uint64_t>());
    max_actions_ = vm[MAX_ACTIONS_CONFIG_NAME].as<std::size_t>();
  } catch (const std::exception& e) {
    std::cout << fmt::format("Default config settings were set because of error during the config file parsing",
        e.what()) << std::endl;
    syslog(LOG_ERR, "%s", fmt::format("Error during the config file parsing: {}", e.what()).c_str());
    syslog(LOG_INFO, "Default settings were set");
    db_path_ = DEFAULT_DB_PATH;
    check_time_ = DEFAULT_CHECK_TIME;
    max_actions_ = DEFAULT_MAX_ACTIONS;
  }
}

} // namespace settings
