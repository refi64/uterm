#include "uterm.h"
#include "window.h"
#include "terminal.h"

#include <SkTypeface.h>
#include <SkFont.h>
#include <xkbcommon/xkbcommon-keysyms.h>

int main() {
  string s(80, ' ');
  std::vector<string> rows(25, s);

  auto draw = [&](const string& str, Pos pos) {
    /* if (str[0]) fmt::print("{} {} {},\n", pos.x, pos.y, str[0]); */
    rows[pos.y][pos.x] = str[0] ? str[0] : ' ';
  };

  Pty pty;
  if (auto err = pty.Spawn({"/bin/bash", "-i"})) {
    err.Extend("while initializing pty").Print();
    return 1;
  }

  Terminal term;
  term.set_draw_cb(draw);
  term.set_pty(&pty);

  Window w;
  if (auto err = w.Initialize(800, 600)) {
    err.Extend("while initializing window").Print();
    return 1;
  }

  sk_sp<SkTypeface> robotoMono{SkTypeface::MakeFromName("Roboto Mono", SkFontStyle{})};

  constexpr SkScalar kFontSize = SkIntToScalar(15);

  SkPaint paint;
  paint.setColor(SK_ColorBLACK);
  paint.setTextSize(kFontSize);
  paint.setAntiAlias(true);
  paint.setTypeface(robotoMono);

  SkPaint::FontMetrics metrics;
  paint.getFontMetrics(&metrics);
  fmt::print("{}\n", metrics.fAvgCharWidth);

  SkRect bounds;
  paint.measureText("x", 1, &bounds);
  fmt::print("{}\n", bounds.width());

  auto key = [&](uint32 keysym, int mods) {
    term.WriteKeysymToPty(keysym, mods);
  };

  auto char_ = [&](uint code) {
    term.WriteUnicodeToPty(code);
  };

  auto resize = [&](int width, int height) {
    int nrows = height / kFontSize;
    int ncols = width / metrics.fAvgCharWidth;

    rows.resize(nrows);
    for (auto& row : rows) {
      row.resize(ncols, ' ');
    }
    term.Resize(ncols, nrows);
  };

  resize(800, 600);

  w.set_key_cb(key);
  w.set_char_cb(char_);
  w.set_resize_cb(resize);

  while (w.isopen()) {
    SkCanvas* canvas = w.canvas();

    if (auto e_text = pty.NonblockingRead()) {
      if (!e_text->empty()) {
        term.WriteToScreen(*e_text);
        term.Draw();
      }
    } else {
      e_text.Error().Print();
      return 1;
    }

    canvas->clear(SK_ColorWHITE);
    for (int i=0; i<rows.size(); i++) {
      auto& row = rows[i];
      canvas->drawText(row.c_str(), row.size(), 0, kFontSize*(i+1), paint);
    }
    w.Draw();
  }

  return 0;
}
