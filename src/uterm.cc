#include "uterm.h"
#include "window.h"

#include <SkTypeface.h>

#include <stdio.h>

int main() {
  Window w;
  if (auto err = w.Initialize(800, 600)) {
    puts(err.trace(0).c_str());
    return 1;
  }

  sk_sp<SkTypeface> robotoMono{SkTypeface::MakeFromName("Roboto Mono", SkFontStyle{})};

  SkPaint paint;
  paint.setColor(SK_ColorBLACK);
  paint.setTextSize(20);
  paint.setAntiAlias(true);
  paint.setTypeface(robotoMono);

  while (w.isopen()) {
    SkCanvas* canvas = w.canvas();

    canvas->clear(SK_ColorWHITE);
    canvas->drawText("Hello, world!", 13, SkIntToScalar(100), SkIntToScalar(100), paint);

    w.Draw();
  }

  return 0;
}
