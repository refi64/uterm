#pragma once

#include <SkTypeface.h>
#include <SkCanvas.h>
#include <SkPaint.h>

#include "base.h"
#include "terminal.h"
#include "text.h"
#include "marker_set.h"

class Display {
public:
  Display(Terminal *term);

  void AddFont(string name, int size);

  void SetSelection(Selection state, int mx, int my);
  void EndSelection();

  Error Resize(int width, int height);
  bool Draw(SkCanvas *canvas, bool lazy_updating);
private:
  void TermDraw(const u32string& str, Pos pos, Attr attr, int width);
  void UpdateWidth();
  void UpdatePositions();
  void UpdateGlyphs();
  void UpdateGlyph(int x, int y);
  void HighlightRange(SkCanvas *canvas, Pos begin, Pos end, SkColor color);

  Terminal *m_term;
  int m_char_width{-1};

  TextManager m_text;
  std::vector<GlyphRenderer> m_renderers;

  using AttrSet = MarkerSet<Attr>;
  AttrSet m_attrs;

  bool m_has_updated{false};
};
