#include "uterm.h"
#include "window.h"
#include "terminal.h"

#include <SkTypeface.h>

void draw(const string& str, Pos pos) {
  fmt::print("{} {} {},", pos.x, pos.y, str[0]);
}

int main() {
  Terminal term;
  term.set_draw(draw);

  term.WriteToScreen("abc");
  term.Draw();
  fmt::print("\n\n\n");
  term.WriteToScreen("d");
  term.Draw();
  fmt::print("\n\n\n");

  return 0;

  /* Window w; */
  /* if (auto err = w.Initialize(800, 600)) { */
  /*   fmt::print("{}\n", err.trace(0)); */
  /*   return 1; */
  /* } */

  /* sk_sp<SkTypeface> robotoMono{SkTypeface::MakeFromName("Roboto Mono", SkFontStyle{})}; */

  /* constexpr SkScalar kFontSize = SkIntToScalar(20); */

  /* SkPaint paint; */
  /* paint.setColor(SK_ColorBLACK); */
  /* paint.setTextSize(kFontSize); */
  /* paint.setAntiAlias(true); */
  /* paint.setTypeface(robotoMono); */

  /* SkCanvas* canvas = w.canvas(); */

  /* while (w.isopen()) { */
  /*   canvas->clear(SK_ColorWHITE); */
  /*   canvas->drawText("Hello, world!", 13, kFontSize, kFontSize, paint); */
  /*   w.Draw(); */
  /* } */

  /* return 0; */
}
