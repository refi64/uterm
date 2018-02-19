#include "pty.h"

#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

class FdWrapper {
public:
  FdWrapper(int fd): m_fd{fd} {}

  ~FdWrapper() {
    if (m_fd != -1) {
      close(m_fd);
    }
  }

  int Relinquish() {
    int orig = -1;
    std::swap(orig, m_fd);
    return orig;
  }
private:
  int m_fd{-1};
};

static Error ChildSpawnTerm(const std::vector<string>& command, int slave) {
  FdWrapper w_slave{slave};

  setenv("TERM", "xterm-256color", 1);

  close(0);
  close(1);
  close(2);

  dup2(slave, 0);
  dup2(slave, 1);
  dup2(slave, 2);

  close(w_slave.Relinquish());

  setsid();
  ioctl(0, TIOCSCTTY, 1);

  std::vector<char*> c_command;
  for (auto& s : command) {
    char* c = new char[s.size()+1];
    std::copy(s.c_str(), s.c_str()+s.size()+1, c);
    c_command.push_back(c);
  }

  execv(c_command[0], c_command.data());

  // If we got this far, then the execv failed.
  return Error::Errno().Extend("execv new process");
}

Pty::~Pty() { Signal(SIGKILL); }

Error Pty::Spawn(const std::vector<string>& command) {
  assert(command.size() >= 1);

  int master = posix_openpt(O_RDWR);
  if (master == -1) {
    return Error::Errno().Extend("opening new master PTY");
  }

  FdWrapper w_master{master};

  if (grantpt(master) == -1) {
    return Error::Errno().Extend("grantpt for master PTY");
  }

  if (unlockpt(master) == -1) {
    return Error::Errno().Extend("unlockpt for master PTY");
  }

  int slave = open(ptsname(master), O_RDWR);
  if (slave == -1) {
    return Error::Errno().Extend("opening slave PTY");
  }

  FdWrapper w_slave{slave};

  int pid = fork();

  if (pid == -1) {
    return Error::Errno().Extend("forking to PTY process");
  } else if (pid == 0) {
    // Slave side.
    close(w_master.Relinquish());
    auto err = ChildSpawnTerm(command, w_slave.Relinquish());

    if (err) {
      err.Extend("in Pty::Spawn slave process").Print();
      exit(1);
    }

    exit(0);
  } else {
    // Master side.
    close(w_slave.Relinquish());
    m_master = w_master.Relinquish();
    m_pid = pid;
    return Error::New();
  }
}

Expect<string> Pty::Read(bool& eof) {
  pollfd poll_master;
  poll_master.fd = m_master;
  poll_master.events = POLLIN;
  poll_master.revents = 0;

  int polled = poll(&poll_master, 1, -1);
  if (polled == -1) {
    if (errno == EINTR) {
      return Expect<string>::New(string(""));
    } else {
      return Expect<string>::New(Error::Errno().Extend("polling master PTY"));
    }
  } else if (poll_master.revents & POLLIN) {
    char buf[4096];
    int sz = read(m_master, buf, sizeof(buf));

    if (sz == -1) {
      return Expect<string>::New(Error::Errno().Extend("reading master PTY"));
    }

    return Expect<string>::New(string(buf, sz));
  } else if (poll_master.revents & (POLLERR | POLLHUP)) {
    eof = true;
    return Expect<string>::New(string(""));
  } else {
    return Expect<string>::WithError("unknown error occurred in NonblockingRead");
  }
}

Error Pty::Write(const string& data) {
  if (write(m_master, static_cast<const void*>(data.c_str()), data.size()) == -1) {
    return Error::Errno().Extend("writing to master PTY");
  }

  return Error::New();
}

Error Pty::Signal(int signal) {
  if (m_pid == -1) {
    return Error::New("cannot Signal Pty without process");
  }

  if (killpg(m_pid, signal) == -1) {
    return Error::Errno().Extend("signaling Pty process");
  }

  return Error::New();
}

Error Pty::Resize(int x, int y) {
  winsize ws;
  ws.ws_xpixel = ws.ws_ypixel = 0;
  ws.ws_row = y;
  ws.ws_col = x;

  if (ioctl(m_master, TIOCSWINSZ, &ws) == -1) {
    return Error::Errno().Extend("resizing pty terminal");
  } else {
    return Error::New();
  }
}
