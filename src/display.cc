#include "display.h"

Display::Display(Terminal *term): m_term{term}, m_attrs{m_term->default_attr()} {
  using namespace std::placeholders;
  m_term->set_draw_cb(std::bind(&Display::TermDraw, this, _1, _2, _3, _4));
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

Error Display::Resize(int width, int height) {
  assert(m_char_width != -1);

  int rows = (height - (m_primary.GetHeight() - m_primary.GetHeight())) /
             m_primary.GetHeight();
  int cols = width / m_char_width;

  m_fallbacks.resize(rows * cols);

  m_text.Resize(cols, rows);

  m_primary.Resize(rows * cols);
  m_fallback.Resize(rows * cols);
  m_attrs.Resize(rows * cols);

  auto err = m_term->Resize(cols, rows);

  UpdatePositions();
  m_has_updated = true;

  if (err) {
    return err.Extend("while resizing display");
  } else {
    return Error::New();
  }
}

bool Display::Draw(SkCanvas *canvas) {
  bool significant_redraw = m_has_updated;

  std::vector<AttrSet::Span> dirty;
  AttrSet::Span *pspan = nullptr;
  while ((pspan = m_attrs.NextSpan(pspan))) {
    if (!pspan->data.dirty) continue;

    dirty.push_back(*pspan);

    SkColor background;
    if (pspan->data.flags & Attr::kInverse) {
      background = pspan->data.foreground;
    } else {
      background = pspan->data.background;
    }

    HighlightRange(canvas, m_text.OffsetToPos(pspan->begin),
                   m_text.OffsetToPos(pspan->end), background);
  }

  for (auto &span : dirty) {
    m_text.DrawRangeWithRenderer(canvas, &m_primary, span.data, span.begin, span.end);
    m_text.DrawRangeWithRenderer(canvas, &m_fallback,  span.data, span.begin, span.end);

    Attr attr = span.data;
    attr.dirty = false;
    m_attrs.Update(span.begin, span.end, attr);
  }

  #ifdef UTERM_BLACK_SCREEN_WORKAROUND
  // Workaround a nasty driver bug where the entire screen turns black if something
  // being redrawn each frame.
  Attr first_cell_attr = m_attrs.At(0);
  first_cell_attr.dirty = true;
  m_attrs.Update(0, first_cell_attr);
  #endif

  m_has_updated = false;
  return significant_redraw;
}

void Display::TermDraw(const u32string& str, Pos pos, Attr attr, int width) {
  m_text.set_cell(pos.x, pos.y, str[0] ? str[0] : ' ');
  UpdateGlyph(pos.x, pos.y);

  attr.dirty = true;
  m_attrs.Update(m_text.PosToOffset(pos), attr);

  m_has_updated = true;
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
  int index = m_text.PosToOffset(x, y);
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

  for (int y = begin.y; y <= end.y; y++) {
    int first = y == begin.y ? begin.x : 0,
        last = y == end.y ? end.x : m_text.cols();
    SkRect rect = SkRect::MakeXYWH(m_char_width * first,
                                   m_primary.GetHeight() * y +
                                    m_primary.GetBaselineOffset(),
                                   m_char_width * (last - first),
                                   m_primary.GetHeight());
    canvas->save();
    canvas->clipRect(rect, false);
    canvas->clear(color);
    canvas->restore();
  }
}
