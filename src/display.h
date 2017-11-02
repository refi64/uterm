#pragma once

#include <SkTypeface.h>
#include <SkCanvas.h>
#include <SkPaint.h>

#include "uterm.h"
#include "terminal.h"

class Display {
public:
  Display(Terminal *term);

  void SetTextSize(int size);
  void SetPrimaryFont(string name);
  void SetFallbackFont(string name);

  void Resize(int width, int height);
  void Draw(SkCanvas *canvas);
private:
  void TermDraw(const u32string& str, Pos pos, int width);
  void UpdateFont(SkPaint *paint, sk_sp<SkTypeface> *font, string name);
  void UpdateWidth();

  Terminal *m_term;
  sk_sp<SkTypeface> m_primary_font;
  SkPaint m_primary_paint;
  int m_char_width{-1};

  sk_sp<SkTypeface> m_fallback_font;
  SkPaint m_fallback_paint;

  std::vector<u32string> m_rows;
};
