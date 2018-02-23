#pragma once

#include "base.h"
#include "error.h"

// A Pty represents a currently active pty (surprise, surprise).
class Pty {
public:
  ~Pty();

  // Spawn the given command within this pty.
  Error Spawn(const std::vector<string>& command);

  // Performs a blocking read from the pty output. If an EOF occurs, returns an empty
  // string.
  Expect<string> Read(bool *eof);
  // Performs a blocking write
  Error Write(const string& data);
  // Sends the given signal to the pty.
  Error Signal(int signal);
  // Resizes the given pty to the number of columns and rows.
  Error Resize(int x, int y);
private:
  // The master end of the pty pipe.
  int m_master{-1};
  // The child process's PID.
  int m_pid{-1};
};
