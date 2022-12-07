#pragma once

#include <chrono>
#include <filesystem>
#include <string>

#include <boost/program_options.hpp>
#include <fmt/format.h>


namespace settings {

  const std::string PROJECT_FILES_PATH = "/etc/makvv_antivirus";
  const auto DEFAULT_CONFIG_PATH = fmt::format("{}/config.ini", PROJECT_FILES_PATH);

  class Config {
   public:
    Config(const std::string& path = settings::DEFAULT_CONFIG_PATH);

    const std::filesystem::path& GetDbPath() const { return db_path_; }
    const std::chrono::milliseconds& GetCheckTime() const { return check_time_; }
    const std::size_t& GetMaxActions() const { return max_actions_; }
   private:
    boost::program_options::options_description desc_;
    std::filesystem::path db_path_;
    std::chrono::milliseconds check_time_;
    std::size_t max_actions_;
  };

} // namespace

