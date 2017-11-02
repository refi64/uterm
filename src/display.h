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
  void UpdatePositions();
  void UpdateGlyphs();
  void UpdateGlyph(int x, int y);

  Terminal *m_term;
  sk_sp<SkTypeface> m_primary_font;
  SkPaint m_primary_paint;
  int m_char_width{-1};

  sk_sp<SkTypeface> m_fallback_font;
  SkPaint m_fallback_paint;

  u32string m_text;
  std::vector<SkPoint> m_text_positions;
  std::vector<SkGlyphID> m_primary_glyphs, m_fallback_glyphs;
  std::vector<bool> m_fallbacks;
  int m_rows, m_cols;
};
