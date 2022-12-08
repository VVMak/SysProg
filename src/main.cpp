#include <fcntl.h>
#include <poll.h>
#include <sys/fanotify.h>
#include <unistd.h>
#include <syslog.h>

#include <cassert>
#include <iostream>

#include <fmt/format.h>

#include <database.hpp>
#include <file.hpp>
#include <process.hpp>

#include <settings.hpp>
#include <utils.hpp>


void HandleWrite(const fanotify_event_metadata* metadata, DatabasePtr& db, const settings::Config& config) {
  const auto path = utils::GetFilePathByFd(metadata->fd).string();
  File target(path, db);

  try {
    Process p(metadata->pid, db);
    db->AddAction(p, target);
    if (p.HasWriteAccess(config)) {
      return;
    }
    utils::BanProcessAndRestoreFiles(db, p);
  } catch (const std::runtime_error& e) {
    syslog(LOG_ERR, "%s", fmt::format(
        "Can't handle write of process {} in file {}, error: {}",
        metadata->pid, path, e.what()
    ).c_str());
  }
}

void HandleOpen(int fd, const fanotify_event_metadata* metadata, DatabasePtr& db, const settings::Config& config) {
  const auto path = utils::GetFilePathByFd(metadata->fd).string();
  File target(path, db);

  try {
    Process p(metadata->pid, db);

    if (p.HasWriteAccess(config)) {
      if (!db->FileWasSaved(target)) {
        db->SaveFileContent(target);
      }
    } else {
      utils::BanProcessAndRestoreFiles(db, p);
    }
  } catch (const std::runtime_error& e) {
    syslog(LOG_ERR, "%s", fmt::format(
        "Can't handle write of process {} in file {}, error: {}",
        metadata->pid, path, e.what()
    ).c_str());
  }
}

void HandleEvents(int fd, DatabasePtr& db, const settings::Config& config) {
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
          HandleWrite(metadata, db, config);
        }
        if (metadata->mask & FAN_OPEN) {
          HandleOpen(fd, metadata, db, config);
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

  utils::CreateDefaultFiles();
  settings::Config config;
  auto db = std::make_shared<Database>(config.GetDbPath());
  db->GetWhiteList()->Add(db->GetFileByPath(std::filesystem::read_symlink("/proc/self/exe")));

  std::cout << "Press enter key to terminate." << std::endl;
  int fd = fanotify_init(FAN_CLOEXEC | FAN_CLASS_PRE_CONTENT | FAN_NONBLOCK,
                      O_RDONLY | O_LARGEFILE);
  if (fd == -1) {
    std::perror("fanotify_init");
    std::exit(EXIT_FAILURE);
  }
  if (fanotify_mark(fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
                    FAN_OPEN | FAN_CLOSE_WRITE, AT_FDCWD,
                    argv[1]) == -1) {
    std::perror("fanotify_mark");
    std::exit(EXIT_FAILURE);
  }
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
        HandleEvents(fd, db, config);
      }
    }
  }

  std::cout << "Listening for events stopped.\n";
  std::exit(EXIT_SUCCESS);
}
