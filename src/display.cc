#include "display.h"

Display::Display(Terminal *term): m_term{term} {
  m_primary_paint.setColor(SK_ColorWHITE);
  m_primary_paint.setAntiAlias(true);
  m_primary_paint.setTextEncoding(SkPaint::kUTF32_TextEncoding);
  m_primary_paint.setSubpixelText(true);

  m_fallback_paint.setColor(SK_ColorWHITE);
  m_fallback_paint.setAntiAlias(true);
  m_fallback_paint.setTextEncoding(SkPaint::kUTF32_TextEncoding);
  m_primary_paint.setSubpixelText(true);

  using namespace std::placeholders;
  m_term->set_draw_cb(std::bind(&Display::TermDraw, this, _1, _2, _3));
}

void Display::SetTextSize(int size) {
  m_primary_paint.setTextSize(SkIntToScalar(size));
  m_fallback_paint.setTextSize(SkIntToScalar(size));
  UpdateWidth();
  UpdateGlyphs();
}

void Display::SetPrimaryFont(string name) {
  UpdateFont(&m_primary_paint, &m_primary_font, name);
  UpdateWidth();
  UpdateGlyphs();
}

void Display::SetFallbackFont(string name) {
  UpdateFont(&m_fallback_paint, &m_fallback_font, name);
}

void Display::Resize(int width, int height) {
  assert(m_char_width != -1);

  m_rows = height / m_primary_paint.getTextSize();
  m_cols = width / m_char_width;

  m_text.resize(m_rows*m_cols, ' ');
  m_text_positions.resize(m_rows*m_cols);
  m_primary_glyphs.resize(m_rows*m_cols);
  m_fallback_glyphs.resize(m_rows*m_cols);
  m_fallbacks.resize(m_rows*m_cols);
  m_term->Resize(m_cols, m_rows);
  UpdatePositions();
}

void Display::Draw(SkCanvas *canvas) {
  auto height = m_primary_paint.getTextSize();

  m_primary_paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
  canvas->drawPosText(m_primary_glyphs.data(),
                      m_primary_glyphs.size() * sizeof(m_primary_glyphs[0]),
                      m_text_positions.data(), m_primary_paint);
  m_fallback_paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
  canvas->drawPosText(m_fallback_glyphs.data(),
                      m_fallback_glyphs.size() * sizeof(m_fallback_glyphs[0]),
                      m_text_positions.data(), m_fallback_paint);
}

void Display::TermDraw(const u32string& str, Pos pos, int width) {
  char32_t *cell = &m_text[pos.y * m_cols + pos.x];
  *cell = str[0] ? str[0] : ' ';
  UpdateGlyph(pos.x, pos.y);
}

void Display::UpdateFont(SkPaint *paint, sk_sp<SkTypeface> *font, string name) {
  *font = SkTypeface::MakeFromName(name.c_str(), SkFontStyle{});
  paint->setTypeface(*font);
}

void Display::UpdateWidth() {
  SkPaint::FontMetrics metrics;
  m_primary_paint.getFontMetrics(&metrics);
  if (metrics.fAvgCharWidth) {
    m_char_width = metrics.fAvgCharWidth;
    return;
  }

  SkRect bounds;
  u32string s{'x'};
  m_primary_paint.measureText(s.c_str(), sizeof(s[0]), &bounds);
  m_char_width = bounds.width();

  UpdatePositions();
}

void Display::UpdatePositions() {
  assert(m_text.size() == m_text_positions.size());
  int height = m_primary_paint.getTextSize();

  for (int i=0; i<m_rows; i++) {
    for (int j=0; j<m_cols; j++) {
      SkPoint *point = &m_text_positions[i * m_cols + j];
      point->fX = m_char_width * j;
      point->fY = height * (i + 1);
    }
  }
}

void Display::UpdateGlyphs() {
  for (int i=0; i<m_rows; i++) {
    for (int j=0; j<m_cols; j++) {
      UpdateGlyph(i, j);
    }
  }
}

void Display::UpdateGlyph(int x, int y) {
  int index = y * m_cols + x;

  m_primary_paint.setTextEncoding(SkPaint::kUTF32_TextEncoding);
  m_primary_paint.textToGlyphs(&m_text[index], sizeof(m_text[0]),
                               &m_primary_glyphs[index]);

  m_fallback_paint.setTextEncoding(SkPaint::kUTF32_TextEncoding);
  if (!m_primary_glyphs[index]) {
    m_fallback_paint.textToGlyphs(&m_text[index], sizeof(m_text[0]),
                                  &m_fallback_glyphs[index]);
    m_fallbacks[index] = true;
  } else {
    char32_t space = ' ';
    m_fallback_paint.textToGlyphs(&space, sizeof(space), &m_fallback_glyphs[index]);
  }
}
