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

  Terminal term;
  term.set_pty(&pty);

  Display disp{&term};

  Window w;
  if (auto err = w.Initialize(800, 600)) {
    err.Extend("while initializing window").Print();
    return 1;
  }

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
    disp.Resize(width, height);
  };

  resize(800, 600);

  w.set_key_cb(key);
  w.set_char_cb(char_);
  w.set_resize_cb(resize);

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

    canvas->clear(SK_ColorBLACK);
    disp.Draw(canvas);
    w.Draw();
  }

  return 0;
}
