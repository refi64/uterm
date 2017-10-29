#include "uterm.h"
#include "window.h"

#include <SkTypeface.h>
#include <libtsm.h>

#include <stdio.h>

/* void write(tsm_vte *vte, const char *u8, size_t len, void *data) { */
/*   return; */
/* } */

/* int draw(tsm_screen *screen, uint32_t id, const uint32_t *chars, size_t len, */
/*          unsigned int width, unsigned int posx, unsigned int posy, */
/*          const tsm_screen_attr *attr, tsm_age_t age, void* data) { */
/*   fmt::print("{} {} {} {},", posx, posy, chars[0], len); */

/*   return 0; */
/* } */

int main() {
  /* tsm_screen* screen = NULL; */
  /* tsm_screen_new(&screen, NULL, NULL); */

  /* tsm_vte* vte = NULL; */
  /* tsm_vte_new(&vte, screen, write, NULL, NULL, NULL); */
  /* tsm_vte_input(vte, "abc", 3); */

  /* tsm_screen_draw(screen, draw, NULL); */
  /* fmt::print("\n"); */

  /* return 0; */

  Window w;
  if (auto err = w.Initialize(800, 600)) {
    fmt::print("{}\n", err.trace(0));
    return 1;
  }

  sk_sp<SkTypeface> robotoMono{SkTypeface::MakeFromName("Roboto Mono", SkFontStyle{})};

  constexpr SkScalar kFontSize = SkIntToScalar(20);

  SkPaint paint;
  paint.setColor(SK_ColorBLACK);
  paint.setTextSize(kFontSize);
  paint.setAntiAlias(true);
  paint.setTypeface(robotoMono);

  SkCanvas* canvas = w.canvas();

  while (w.isopen()) {
    canvas->clear(SK_ColorWHITE);
    canvas->drawText("Hello, world!", 13, kFontSize, kFontSize, paint);
    w.Draw();
  }

  return 0;
}
