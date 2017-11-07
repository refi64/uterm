#pragma once

#include "uterm.h"
#include "error.h"

class Pty {
public:
  ~Pty();
  Error Spawn(const std::vector<string>& command);

  Expect<string> NonblockingRead();
  Error Write(const string& data);
  Error Signal(int signal);
  Error Resize(int x, int y);
private:
  int m_master{-1};
  int m_pid{-1};
};
