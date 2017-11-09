#include "uterm.h"
#include "window.h"
#include "terminal.h"
#include "display.h"

#include <xkbcommon/xkbcommon-keysyms.h>

int main() {
  Pty pty;
  if (auto err = pty.Spawn({"/bin/bash", "-i"})) {
    err.Extend("while initializing pty").Print();
    return 1;
  }

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

  resize(800, 600);

  w.set_key_cb(key);
  w.set_char_cb(char_);
  w.set_resize_cb(resize);
  w.set_selection_cb(selection);

  SkCanvas *canvas = w.canvas();
  canvas->clear(SK_ColorBLACK);

  while (w.isopen()) {
    SkCanvas *canvas = w.canvas();

    if (auto e_text = pty.NonblockingRead()) {
      if (!e_text->empty()) {
        term.WriteToScreen(*e_text);
        term.Draw();
      }
    } else {
      auto err = e_text.Error();
      if (err.trace(0).find("EOF") != -1) {
        break;
      }
      e_text.Error().Print();
      continue;
    }

    w.DrawAndPoll(disp.Draw(canvas));
  }

  return 0;
}
