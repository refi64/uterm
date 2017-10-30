#include "uterm.h"
#include "window.h"
#include "terminal.h"

#include <SkTypeface.h>

std::vector<string>* rows;

void draw(const string& str, Pos pos) {
  (*rows)[pos.y][pos.x] = str[0];
  /* fmt::print("{} {} {},", pos.x, pos.y, str[0]); */
}

int main() {
  string s(80, ' ');
  rows = new std::vector<string>(25, s);

  Pty pty;
  if (auto err = pty.Spawn({"/bin/bash", "-i"})) {
    fmt::print("{}\n", err.trace(0));
    return 1;
  }

  Terminal term;
  term.set_draw(draw);
  term.set_pty(&pty);

  Window w;
  if (auto err = w.Initialize(800, 600)) {
    fmt::print("{}\n", err.trace(0));
    return 1;
  }

  sk_sp<SkTypeface> robotoMono{SkTypeface::MakeFromName("Roboto Mono", SkFontStyle{})};

  constexpr SkScalar kFontSize = SkIntToScalar(15);

  SkPaint paint;
  paint.setColor(SK_ColorBLACK);
  paint.setTextSize(kFontSize);
  paint.setAntiAlias(true);
  paint.setTypeface(robotoMono);

  SkCanvas* canvas = w.canvas();

  while (w.isopen()) {
    if (auto e_text = pty.NonblockingRead()) {
      if (!e_text->empty()) {
        term.WriteToScreen(*e_text);
        term.Draw();
      }
    } else {
      auto err = e_text.Error();
      fmt::print("{}\n", err.trace(0));
      return 1;
    }

    canvas->clear(SK_ColorWHITE);
    for (int i=0; i<rows->size(); i++) {
      auto& row = (*rows)[i];
      canvas->drawText(row.c_str(), row.size(), 0, kFontSize*(i+1), paint);
    }
    w.Draw();
  }

  return 0;
}
