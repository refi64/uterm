#include "uterm.h"

#include <sys/wait.h>
#include <signal.h>

void ProtectedBuffer::Append(string text) {
  std::unique_lock<std::mutex> lock{m_lock};
  m_buffer += text;
}

string ProtectedBuffer::ReadAndClear() {
  std::unique_lock<std::mutex> lock{m_lock};
  string result = m_buffer;
  m_buffer.clear();
  return result;
}

ReaderThread::ReaderThread(Pty *pty): m_thread{&ReaderThread::StaticRun, this, pty} {}

void ReaderThread::Stop() {
  m_done_flag.set();
  // Interrupt waiting polls.
  pthread_kill(m_thread.native_handle(), SIGUSR1);
  m_thread.join();
}

void ReaderThread::StaticRun(Pty *pty) {
  bool eof = false;
  while (!m_done_flag.get()) {
    if (auto e_text = pty->Read(eof)) {
      if (!e_text->empty()) {
        m_buffer.Append(*e_text);
        // Do a short (0.5ms) sleep to avoid high CPU usage because of short polls.
        usleep(500);
      } else if (eof) {
        m_done_flag.set();
      }
    } else {
      e_text.Error().Extend("reading data from pty").Print();
    }
  }
}

static void CatchSigchld(int sig) {
  waitpid(-1, NULL, WNOHANG);
}

int Uterm::Run() {
  using namespace std::placeholders;

  if (auto err = m_config.Parse()) {
    err.Extend("while parsing config file").Print();
  }

  constexpr int kWidth = 800, kHeight = 600;

  signal(SIGCHLD, CatchSigchld);
  signal(SIGUSR1, [](int sig) {});

  Pty pty;
  if (auto err = pty.Spawn({m_config.shell(), "-i"})) {
    err.Extend("while initializing pty").Print();
    return 1;
  }

  ReaderThread reader{&pty};

  if (auto err = m_window.Initialize(kWidth, kHeight, m_config.theme())) {
    err.Extend("while initializing window").Print();
    return 1;
  }

  m_term.set_theme(m_config.theme());

  m_term.set_pty(&pty);
  m_term.set_copy_cb(std::bind(&Uterm::HandleCopy, this, _1));
  m_term.set_paste_cb(std::bind(&Uterm::HandlePaste, this));

  for (auto &font : m_config.fonts()) {
    m_display.AddFont(font.name, font.size);
  }

  m_display.AddFont("monospace", m_config.font_defaults_size());

  HandleResize(kWidth, kHeight);

  m_window.set_key_cb(std::bind(&Uterm::HandleKey, this, _1, _2));
  m_window.set_char_cb(std::bind(&Uterm::HandleChar, this, _1));
  m_window.set_resize_cb(std::bind(&Uterm::HandleResize, this, _1, _2));
  m_window.set_selection_cb(std::bind(&Uterm::HandleSelection, this, _1, _2, _3));
  m_window.set_scroll_cb(std::bind(&Uterm::HandleScroll, this, _1, _2));

  while (m_window.isopen() && !reader.done()) {
    SkCanvas *canvas = m_window.canvas();

    string buffer = reader.buffer().ReadAndClear();
    if (!buffer.empty()) {
      m_term.WriteToScreen(buffer);
    }

    m_term.Draw();

    bool significant_redraw = m_display.Draw(canvas);
    m_window.DrawAndPoll(significant_redraw);
  }

  reader.Stop();
  return 0;
}

void Uterm::HandleCopy(const string &str) {
  m_window.ClipboardWrite(str);
}

string Uterm::HandlePaste() {
  return m_window.ClipboardRead();
}

void Uterm::HandleKey(uint32 keysym, int mods) {
  m_term.WriteKeysymToPty(keysym, mods);
}

void Uterm::HandleChar(uint code) {
  m_term.WriteUnicodeToPty(code);
}

void Uterm::HandleResize(int width, int height) {
  if (auto err = m_display.Resize(width, height)) {
    err.Extend("while resizing terminal display").Print();
  }
}

void Uterm::HandleSelection(Selection state, double mx, double my) {
  if (state == Selection::kEnd) {
    m_display.EndSelection();
  } else {
    m_display.SetSelection(state, mx, my);
  }
}

void Uterm::HandleScroll(ScrollDirection direction, uint distance) {
  m_term.Scroll(direction, distance);
}
