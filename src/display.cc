#include "display.h"

Display::Display(Terminal *term): m_term{term} {
  m_primary_paint.setColor(SK_ColorWHITE);
  m_primary_paint.setAntiAlias(true);
  m_primary_paint.setTextEncoding(SkPaint::kUTF32_TextEncoding);

  m_fallback_paint.setColor(SK_ColorWHITE);
  m_fallback_paint.setAntiAlias(true);
  m_fallback_paint.setTextEncoding(SkPaint::kUTF32_TextEncoding);

  using namespace std::placeholders;
  m_term->set_draw_cb(std::bind(&Display::TermDraw, this, _1, _2, _3));
}

void Display::SetTextSize(int size) {
  m_primary_paint.setTextSize(SkIntToScalar(size));
  m_fallback_paint.setTextSize(SkIntToScalar(size));
  UpdateWidth();
}

void Display::SetPrimaryFont(string name) {
  UpdateFont(&m_primary_paint, &m_primary_font, name);
  UpdateWidth();
}

void Display::SetFallbackFont(string name) {
  UpdateFont(&m_fallback_paint, &m_fallback_font, name);
}

void Display::Resize(int width, int height) {
  assert(m_char_width != -1);

  int nrows = height / m_primary_paint.getTextSize();
  int ncols = width / m_char_width;

  m_rows.resize(nrows);
  for (auto& row : m_rows) {
    row.resize(ncols, ' ');
  }

  m_term->Resize(ncols, nrows);
}

void Display::Draw(SkCanvas *canvas) {
  auto height = m_primary_paint.getTextSize();

  for (int i=0; i<m_rows.size(); i++) {
    auto &row = m_rows[i];
    canvas->drawText(row.c_str(), row.size() * sizeof(row[0]), 0, height * (i + 1),
                     m_primary_paint);
  }
}

void Display::TermDraw(const u32string& str, Pos pos, int width) {
  assert(pos.y < m_rows.size());
  auto &row = m_rows[pos.y];
  assert(pos.x < row.size());
  row[pos.x] = str[0] ? str[0] : ' ';
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
}
