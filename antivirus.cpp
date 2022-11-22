#include <fcntl.h>
#include <poll.h>
#include <sys/fanotify.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <queue>

using namespace std::chrono_literals;
class ActionStorage {
  static constexpr std::size_t MAX_ACTIONS_IN_SAME_DIR = 3;
  static constexpr std::chrono::duration CHECKING_INTERVAL = 1000ms;
 public:
  void Add(int pid, const std::filesystem::path& path);
  bool Check(int pid);
 private:
  using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
  using FilesUsages = std::unordered_map<std::string, TimePoint>;
  using DirsUsages = std::unordered_map<std::string, FilesUsages>; 
  // Для каждого PID, директории, файла, храним последнее время изменения соотв. файла
  // Т.е. снаружи мапа из PID, внутри мапа из директорий, ещё внутри из файла в time_point
  std::unordered_map<int, DirsUsages> pids_actions_;
  std::unordered_set<int> banned_pids_;

  void TruncateTimeQueue(int pid, const std::string& path, const TimePoint& tp);
};

void ActionStorage::Add(int pid, const std::filesystem::path& path) {
  const auto now = std::chrono::steady_clock::now();
  pids_actions_[pid][path.parent_path()][path.filename()] = now;
  TruncateTimeQueue(pid, path, now);
}

bool ActionStorage::Check(int pid) {
  if (banned_pids_.contains(pid)) {
    return false;
  }
  if (!pids_actions_.contains(pid)) {
    return true;
  }
  bool check = true;
  for (auto& [_, files_usages] : pids_actions_[pid]) {
    if (files_usages.size() > MAX_ACTIONS_IN_SAME_DIR) {
      banned_pids_.insert(pid);
      check = false;
      break;
    }
  }
  if (!check) {
    pids_actions_.erase(pid);
  }
  return check;
}

void ActionStorage::TruncateTimeQueue(int pid, const std::string& path, const TimePoint& tp) {
  auto file_usages = pids_actions_[pid][path];
  for (auto it = file_usages.begin(); it != file_usages.end();) {
    if (tp - it->second > CHECKING_INTERVAL) {
      it = file_usages.erase(it);
    } else {
      ++it;
    }
  }
  if (file_usages.empty()) {
    pids_actions_[pid].erase(path);
  }
  if (pids_actions_[pid].empty()) {
    pids_actions_.erase(pid);
  }
}

void handle_write(const fanotify_event_metadata* metadata, ActionStorage& storage) {
  const auto path = std::filesystem::read_symlink("/proc/self/fd/" + std::to_string(metadata->fd));
  storage.Add(metadata->pid, path);
  if (!storage.Check(metadata->pid)) {
    std::cout << "DANGEROUS PROCESS WITH ID " + std::to_string(metadata->pid) << std::endl;
    // I can write something like 'std::system("kill -9 " + std::to_string(metadata->pid));' here,
    // but in educational process it doesn't matter I think, in this program I just forbid next opens
    // (for both read/write and executing)
  }
}

void handle_open(int fd, const fanotify_event_metadata* metadata, ActionStorage& storage) {
  struct fanotify_response response;
  response.fd = metadata->fd;
  response.response = (storage.Check(metadata->pid) ? FAN_ALLOW : FAN_DENY);
  write(fd, &response, sizeof(response));
}

void handle_events(int fd, ActionStorage& storage) {
  const fanotify_event_metadata* metadata;
  fanotify_event_metadata buf[200];
  int len;

  for (;;) {
    len = read(fd, buf, sizeof(buf));
    if (len == -1 && errno != EAGAIN) {
      std::perror("read");
      std::exit(EXIT_FAILURE);
    }
    if (len <= 0) {
      break;
    }
    metadata = buf;
    while (FAN_EVENT_OK(metadata, len)) {
      if (metadata->vers != FANOTIFY_METADATA_VERSION) {
        std::cerr << "Mismatch of fanotify metadata version." << std::endl;
        std::exit(EXIT_FAILURE);
      }
      if (metadata->fd >= 0) {
        if (metadata->mask & FAN_CLOSE_WRITE) {
          handle_write(metadata, storage);
        }
        if (metadata->mask & FAN_OPEN_PERM || metadata->mask & FAN_OPEN_EXEC_PERM) {
          handle_open(fd, metadata, storage);
        }
        close(metadata->fd);
      }
      metadata = FAN_EVENT_NEXT(metadata, len);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " MOUNT\n";
    std::exit(EXIT_FAILURE);
  }
  std::cout << "Press enter key to terminate." << std::endl;
  int fd = fanotify_init(FAN_CLOEXEC | FAN_CLASS_PRE_CONTENT | FAN_NONBLOCK,
                      O_RDONLY | O_LARGEFILE);
  if (fd == -1) {
    std::perror("fanotify_init");
    std::exit(EXIT_FAILURE);
  }
  if (fanotify_mark(fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
                    FAN_OPEN_PERM | FAN_OPEN_EXEC_PERM | FAN_CLOSE_WRITE, AT_FDCWD,
                    argv[1]) == -1) {
    std::perror("fanotify_mark");
    std::exit(EXIT_FAILURE);
  }

  ActionStorage storage;
  int poll_num;
  nfds_t nfds;
  pollfd fds[2];

  nfds = 2;
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;
  fds[1].fd = fd;
  fds[1].events = POLLIN;

  std::cout << "Listening for events." << std::endl;
  while (1) {
    poll_num = poll(fds, nfds, -1);
    if (poll_num == -1) {
      if (errno == EINTR)     /* Interrupted by a signal */
        continue;           /* Restart poll() */

      std::perror("poll");
      std::exit(EXIT_FAILURE);
    }

    if (poll_num > 0) {
      if (fds[0].revents & POLLIN) {
        break;
      }

      if (fds[1].revents & POLLIN) {
        handle_events(fd, storage);
      }
    }
  }

  std::cout << "Listening for events stopped.\n";
  std::exit(EXIT_SUCCESS);
}
