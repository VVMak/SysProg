#include <fcntl.h>
#include <poll.h>
#include <sys/fanotify.h>
#include <unistd.h>

#include <cassert>
#include <iostream>

#include <database.hpp>
#include <file.hpp>
#include <process.hpp>

#include <utils.hpp>

void HandleWrite(const fanotify_event_metadata* metadata, DatabasePtr& db) {
  const auto path = std::filesystem::read_symlink("/proc/self/fd/" + std::to_string(metadata->fd)).string();
  File target(path, db);
  Process p(metadata->pid, db);

  db->AddAction(p, target);
  if (p.HasWriteAccess()) {
    return;
  }
  utils::BanProcessAndRestoreFiles(db, p);
}

void HandleOpen(int fd, const fanotify_event_metadata* metadata, DatabasePtr& db) {
  const auto path = std::filesystem::read_symlink("/proc/self/fd/" + std::to_string(metadata->fd)).string();
  File target(path, db);
  Process p(metadata->pid, db);

  struct fanotify_response response;
  response.fd = metadata->fd;
  if (target.IsAntivirusDb()) {
    response.response = (p.HasAccessToDb() ? FAN_ALLOW : FAN_DENY);
  } else if (p.HasWriteAccess()) {
    if (!db->FileWasSaved(target)) {
      db->SaveFileContent(target);
    }
    response.response = FAN_ALLOW;
  } else {
    response.response = FAN_DENY;
    utils::BanProcessAndRestoreFiles(db, p);
  }
  write(fd, &response, sizeof(response));
}

void HandleEvents(int fd, DatabasePtr& db) {
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
          HandleWrite(metadata, db);
        }
        if (metadata->mask & FAN_OPEN_PERM || metadata->mask & FAN_OPEN_EXEC_PERM) {
          HandleOpen(fd, metadata, db);
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

  auto db = std::make_shared<Database>("/home/makvv/study/sys_prog/test.db");
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
        HandleEvents(fd, db);
      }
    }
  }

  std::cout << "Listening for events stopped.\n";
  std::exit(EXIT_SUCCESS);
}
