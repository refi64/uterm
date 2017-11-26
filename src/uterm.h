#pragma once

#include "window.h"
#include "terminal.h"
#include "display.h"

#include <atomic>
#include <thread>
#include <mutex>

class AtomicFlag {
public:
  bool get() { return m_flag.load(); }
  void set() { m_flag.store(true); }
private:
  std::atomic<bool> m_flag{false};
};

class ProtectedBuffer {
public:
  void Append(string text);
  string ReadAndClear();
private:
  std::mutex m_lock;
  string m_buffer;
};

class ReaderThread {
public:
  ReaderThread(Pty *pty);

  void Stop();

  ProtectedBuffer & buffer() { return m_buffer; }
  bool done() { return m_done_flag.get(); }
private:
  void StaticRun(Pty *pty);

  std::thread m_thread;
  ProtectedBuffer m_buffer;
  AtomicFlag m_done_flag;
};

class Uterm {
public:
  int Run();
private:
  void HandleCopy(const string &str);
  string HandlePaste();
  void HandleKey(uint32 keysym, int mods);
  void HandleChar(uint code);
  void HandleResize(int width, int height);
  void HandleSelection(Selection state, double mx, double my);
  void HandleScroll(ScrollDirection direction, uint distance);

  Terminal m_term;
  Display m_display{&m_term};
  Window m_window;
};
