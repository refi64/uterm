#pragma once

#include <SkTypeface.h>
#include <SkCanvas.h>
#include <SkPaint.h>

#include <array>
#include <limits>

#include "uterm.h"
#include "terminal.h"

// A GlyphRenderer knows little about its textual contents. Its sole goal is to store
// glyphs in a horizontal array, and then render them using the given positions when
// requested.
class GlyphRenderer {
public:
  GlyphRenderer();

  void Resize(int size);
  void SetTextSize(int height);
  void SetFont(string name);
  bool UpdateGlyph(char32_t c, int index);
  void ClearGlyph(int index);
  int GetHeight();
  int GetWidth();
  int GetBaselineOffset();

  void DrawRange(SkCanvas *canvas, SkPoint *positions, Attr attrs, size_t begin,
                 size_t end, bool inverse);
private:
  static constexpr char kCharMax = std::numeric_limits<char>::max();
  void UpdateForFontChange();

  SkPaint m_paint;

  sk_sp<SkTypeface> m_font;
  SkPaint::FontMetrics m_metrics;

  std::vector<SkGlyphID> m_glyphs;
  std::array<SkGlyphID, kCharMax> m_glyph_cache;
};

// A TextManager is the bridge between a terminal's contents and a GlyphRenderer. It
// contains the text itself, as well as the text positions. When drawn, it hands down the
// positions to the GlyphRenderer. Note that a GlyphRenderer does not know when a
// TextManager has updated its cells; the renderer only ever sees the positions.
class TextManager {
public:
  TextManager();

  int rows() { return m_rows; }
  int cols() { return m_cols; }
  char32_t cell(int x, int y);
  void set_cell(int x, int y, char32_t value);
  void Resize(int x, int y);
  void UpdatePositions(int height, int width);

  uint PosToOffset(int x, int y) { return y * m_cols + x; }
  uint PosToOffset(Pos pos) { return PosToOffset(pos.x, pos.y); }
  Pos OffsetToPos(uint offset) { return {offset % m_cols, offset / m_cols}; }

  void DrawRangeWithRenderer(SkCanvas *canvas, GlyphRenderer *renderer, Attr attrs,
                             size_t begin, size_t end, bool inverse);
private:
  uint m_cols{0}, m_rows{0};
  std::u32string m_text;
  std::vector<SkPoint> m_positions;
};
