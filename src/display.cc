#include "display.h"

Display::Display(Terminal *term): m_term{term} {
  using namespace std::placeholders;
  m_term->set_draw_cb(std::bind(&Display::TermDraw, this, _1, _2, _3));
}

void Display::SetTextSize(int size) {
  m_primary.SetTextSize(size);
  m_fallback.SetTextSize(size);
  UpdateWidth();
  UpdateGlyphs();
}

void Display::SetPrimaryFont(string name) {
  m_primary.SetFont(name);
  UpdateWidth();
  UpdateGlyphs();
}

void Display::SetFallbackFont(string name) {
  m_fallback.SetFont(name);
  UpdateGlyphs();
}

void Display::Resize(int width, int height) {
  assert(m_char_width != -1);

  int rows = height / m_primary.GetHeight();
  int cols = width / m_char_width;

  m_fallbacks.resize(rows * cols);

  m_term->Resize(cols, rows);
  m_text.Resize(cols, rows);

  m_primary.Resize(rows * cols);
  m_fallback.Resize(rows * cols);

  UpdatePositions();
}

void Display::Draw(SkCanvas *canvas) {
  m_text.DrawWithRenderer(canvas, &m_primary);
  m_text.DrawWithRenderer(canvas, &m_fallback);
}

void Display::TermDraw(const u32string& str, Pos pos, int width) {
  m_text.set_cell(pos.x, pos.y, str[0] ? str[0] : ' ');
  UpdateGlyph(pos.x, pos.y);
}

void Display::UpdateWidth() {
  m_char_width = m_primary.GetWidth();
  UpdatePositions();
}

void Display::UpdatePositions() {
  m_text.UpdatePositions(m_primary.GetHeight(), m_char_width);
}

void Display::UpdateGlyphs() {
  for (int i=0; i<m_text.rows(); i++) {
    for (int j=0; j<m_text.cols(); j++) {
      UpdateGlyph(i, j);
    }
  }
}

void Display::UpdateGlyph(int x, int y) {
  int index = y * m_text.cols() + x;
  char32_t c = m_text.cell(x, y);

  if (m_primary.UpdateGlyph(c, index)) {
    m_fallbacks[index] = false;
    m_fallback.ClearGlyph(index);
  } else {
    m_fallbacks[index] = true;
    m_fallback.UpdateGlyph(c, index);
  }
}
