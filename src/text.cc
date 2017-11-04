#include "text.h"

GlyphRenderer::GlyphRenderer() {
  m_paint.setColor(SK_ColorWHITE);
  m_paint.setAntiAlias(true);
  m_paint.setTextEncoding(SkPaint::kUTF32_TextEncoding);
  m_paint.setSubpixelText(true);
}

void GlyphRenderer::Resize(int size) {
  m_glyphs.resize(size);
}

void GlyphRenderer::SetTextSize(int height) {
  m_paint.setTextSize(SkIntToScalar(height));
  m_paint.getFontMetrics(&m_metrics);
}

void GlyphRenderer::SetFont(string name) {
  m_font = SkTypeface::MakeFromName(name.c_str(), SkFontStyle{});
  m_paint.setTypeface(m_font);

  char32_t space = ' ';
  m_paint.textToGlyphs(&space, sizeof(space), &m_space_glyph);

  m_paint.getFontMetrics(&m_metrics);
}

bool GlyphRenderer::UpdateGlyph(char32_t c, int index) {
  m_paint.setTextEncoding(SkPaint::kUTF32_TextEncoding);
  m_paint.textToGlyphs(&c, sizeof(c), &m_glyphs[index]);
  return m_glyphs[index] != 0;
}

void GlyphRenderer::ClearGlyph(int index) {
  m_glyphs[index] = m_space_glyph;
}

int GlyphRenderer::GetHeight() {
  return m_paint.getTextSize() + m_metrics.fBottom;
}

int GlyphRenderer::GetWidth() {
  if (m_metrics.fAvgCharWidth) {
    return m_metrics.fAvgCharWidth;
  }

  SkRect bounds;
  u32string s{'x'};
  m_paint.measureText(s.c_str(), sizeof(s[0]), &bounds);
  return bounds.width();
}

int GlyphRenderer::GetBaselineOffset() {
  return m_metrics.fBottom;
}

void GlyphRenderer::DrawRange(SkCanvas *canvas, SkPoint *positions, Attr attrs,
                              size_t begin, size_t end) {
  m_paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
  m_paint.setColor(attrs.foreground);
  canvas->drawPosText(m_glyphs.data() + begin, (end - begin) * sizeof(m_glyphs[0]),
                      positions + begin, m_paint);
}

TextManager::TextManager() {}

void TextManager::Resize(int x, int y) {
  m_rows = y;
  m_cols = x;

  m_text.resize(m_rows * m_cols);
  m_positions.resize(m_rows * m_cols);
}

char32_t TextManager::cell(int x, int y) {
  return m_text[y * m_cols + x];
}

void TextManager::set_cell(int x, int y, char32_t value) {
  m_text[y * m_cols + x] = value;
}

void TextManager::UpdatePositions(int height, int width) {
  assert(m_text.size() == m_positions.size());

  for (int i=0; i<m_rows; i++) {
    for (int j=0; j<m_cols; j++) {
      SkPoint *point = &m_positions[i * m_cols + j];
      point->fX = width * j;
      point->fY = height * (i + 1);
    }
  }
}

void TextManager::DrawRangeWithRenderer(SkCanvas *canvas, GlyphRenderer *renderer,
                                        Attr attrs, size_t begin, size_t end) {
  renderer->DrawRange(canvas, m_positions.data(), attrs, begin, end);
}
