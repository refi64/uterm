#include "display.h"

Display::Display(Terminal *term): m_term{term} {
  using namespace std::placeholders;
  m_term->set_draw_cb(std::bind(&Display::TermDraw, this, _1, _2, _3, _4));

  m_highlight_paint.setAntiAlias(true);
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

  int rows = (height - (m_primary.GetHeight() - m_primary.GetHeight())) /
             m_primary.GetHeight();
  int cols = width / m_char_width;

  m_fallbacks.resize(rows * cols);

  m_text.Resize(cols, rows);

  m_primary.Resize(rows * cols);
  m_fallback.Resize(rows * cols);

  m_term->Resize(cols, rows);

  UpdatePositions();
}

void Display::Draw(SkCanvas *canvas) {
  Pos cursor = m_term->cursor();

  std::vector<AttrSet::Span> dirty;
  AttrSet::Span *pspan = nullptr;
  while ((pspan = m_attrs.NextSpan(pspan))) {
    if (!pspan->data.dirty) continue;

    dirty.push_back(*pspan);
    HighlightRange(canvas, m_text.OffsetToPos(pspan->begin),
                   m_text.OffsetToPos(pspan->end), pspan->data.background);
  }

  for (auto &span : dirty) {
    m_text.DrawRangeWithRenderer(canvas, &m_primary, span.data, span.begin, span.end);
    m_text.DrawRangeWithRenderer(canvas, &m_fallback,  span.data, span.begin, span.end);

    Attr attr = span.data;
    attr.dirty = false;
    m_attrs.Update(span.begin, span.end, attr);
  }

  HighlightRange(canvas, cursor, {cursor.x+1, cursor.y}, SK_ColorWHITE);
}

void Display::TermDraw(const u32string& str, Pos pos, Attr attr, int width) {
  m_text.set_cell(pos.x, pos.y, str[0] ? str[0] : ' ');
  UpdateGlyph(pos.x, pos.y);

  attr.dirty = true;
  m_attrs.Update(pos.y * m_text.cols() + pos.x, attr);
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

void Display::HighlightRange(SkCanvas *canvas, Pos begin, Pos end, SkColor color) {
  assert(begin.y <= end.y);
  m_highlight_paint.setColor(color);

  for (int y = begin.y; y <= end.y; y++) {
    int first = y == begin.y ? begin.x : 0,
        last = y == end.y ? end.x : m_text.cols();
    SkRect rect = SkRect::MakeXYWH(m_char_width * first,
                                   m_primary.GetHeight() * y +
                                    m_primary.GetBaselineOffset(),
                                   m_char_width * (last - first),
                                   m_primary.GetHeight());
    canvas->drawRect(rect, m_highlight_paint);
  }
}
