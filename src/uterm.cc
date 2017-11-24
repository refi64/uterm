#include "uterm.h"
#include "window.h"
#include "terminal.h"
#include "display.h"

#include <atomic>
#include <thread>
#include <mutex>

std::mutex mutex;
string buffer;
std::atomic<bool> do_close{false};

void OtherThread(Pty *pty) {
  while (!do_close.load()) {
    if (auto e_text = pty->Read()) {
      if (!e_text->empty()) {
        std::unique_lock<std::mutex> lock{mutex};
        buffer += *e_text;
      } else {
        do_close.store(true);
      }
    } else {
      auto err = e_text.Error();
      e_text.Error().Extend("reading data from pty").Print();
    }
  }
}

int main() {
  const char* shell = getenv("SHELL");

  Pty pty;
  if (auto err = pty.Spawn({shell ? shell : "/bin/sh", "-i"})) {
    err.Extend("while initializing pty").Print();
    return 1;
  }

  std::thread thread{OtherThread, &pty};

  Window w;
  if (auto err = w.Initialize(800, 600)) {
    err.Extend("while initializing window").Print();
    return 1;
  }

  auto copy = [&](const string& str) {
    w.ClipboardWrite(str);
  };

  auto paste = [&]() -> string {
    return w.ClipboardRead();
  };

  Attr attr;
  attr.foreground = SK_ColorWHITE;
  attr.background = SK_ColorBLACK;
  attr.flags = 0;
  attr.dirty = false;

  Terminal term{attr};
  term.set_pty(&pty);
  term.set_copy_cb(copy);
  term.set_paste_cb(paste);

  Display disp{&term};

  disp.SetPrimaryFont("Roboto Mono");
  disp.SetFallbackFont("Noto Sans");

  constexpr SkScalar kFontSize = SkIntToScalar(15);
  disp.SetTextSize(kFontSize);

  auto key = [&](uint32 keysym, int mods) {
    term.WriteKeysymToPty(keysym, mods);
  };

  auto char_ = [&](uint code) {
    term.WriteUnicodeToPty(code);
  };

  auto resize = [&](int width, int height) {
    if (auto err = disp.Resize(width, height)) {
      err.Print();
    }
  };

  auto selection = [&](Selection state, double mx, double my) {
    if (state == Selection::kEnd) disp.EndSelection();
    else disp.SetSelection(state, mx, my);
  };

  bool needs_redraw{false};

  auto scroll = [&](ScrollDirection direction, uint distance) {
    term.Scroll(direction, distance);
    needs_redraw = true;
  };

  resize(800, 600);

  w.set_key_cb(key);
  w.set_char_cb(char_);
  w.set_resize_cb(resize);
  w.set_selection_cb(selection);
  w.set_scroll_cb(scroll);

  SkCanvas *canvas = w.canvas();
  canvas->clear(SK_ColorBLACK);

  string buf;

  while (w.isopen()) {
    if (do_close.load()) {
      break;
    }

    SkCanvas *canvas = w.canvas();

    string local_buffer;
    {
      std::unique_lock<std::mutex> lock{mutex};
      local_buffer = buffer;
      buffer.clear();
    }

    if (!local_buffer.empty()) {
      term.WriteToScreen(local_buffer);
      needs_redraw = true;
    }

    if (needs_redraw) {
      term.Draw();
    }

    w.DrawAndPoll(disp.Draw(canvas));
  }

  do_close.store(true);
  thread.join();
  return 0;
}
