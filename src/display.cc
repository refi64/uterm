#include "display.h"

// Clamps v to the range low (inclusive) to high (exclusive).
template <typename T>
T clamp(T v, T low, T high) {
  return v < low ? low : (v > high ? high : v);
}

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

void Display::SetSelection(Selection state, int mx, int my) {
  int x = clamp(mx / m_char_width, 0, m_text.cols() - 1);
  int y = clamp(my / m_primary.GetHeight(), 0, m_text.rows() - 1);

  // Reset the old selection...
  auto old_sel = m_term->selection();
  uint old_sel_begin = m_text.PosToOffset(old_sel.begin_x, old_sel.begin_y),
       old_sel_end = m_text.PosToOffset(old_sel.end_x, old_sel.end_y);

  m_attrs.UpdateWith(old_sel_begin, old_sel_end, [](Attr &attr) {
    attr.dirty = true;
    attr.selected = false;
  });

  m_term->SetSelection(state, x, y);

  // ...and mark the new one.
  auto new_sel = m_term->selection();
  uint new_sel_begin = m_text.PosToOffset(new_sel.begin_x, new_sel.begin_y),
       new_sel_end = m_text.PosToOffset(new_sel.end_x, new_sel.end_y);

  m_attrs.UpdateWith(new_sel_begin, new_sel_end, [](Attr &attr) {
    attr.dirty = true;
    attr.selected = true;
  });

  m_has_updated = true;
}

void Display::EndSelection() {
  m_term->EndSelection();
}

Error Display::Resize(int width, int height) {
  assert(m_char_width != -1);

  int rows = height / m_primary.GetHeight();
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

  std::vector<std::pair<AttrSet::Span, bool>> dirty;
  AttrSet::Span *pspan = nullptr;
  while ((pspan = m_attrs.NextSpan(pspan))) {
    if (!pspan->data.dirty) continue;

    bool inverse = (pspan->data.flags ^ (pspan->data.selected ? Attr::kInverse : 0)) &
                   Attr::kInverse;

    SkColor background;
    if (inverse) {
      background = pspan->data.foreground;
    } else {
      background = pspan->data.background;
    }

    HighlightRange(canvas, m_text.OffsetToPos(pspan->begin),
                   m_text.OffsetToPos(pspan->end), background);

    dirty.emplace_back(*pspan, inverse);
  }

  for (auto &sect : dirty) {
    auto &span = sect.first;
    bool inverse = sect.second;

    m_text.DrawRangeWithRenderer(canvas, &m_primary, span.data, span.begin, span.end,
                                 inverse);
    m_text.DrawRangeWithRenderer(canvas, &m_fallback,  span.data, span.begin, span.end,
                                 inverse);

    m_attrs.UpdateWith(span.begin, span.end, [](Attr &attr) {
      attr.dirty = false;
    });
  }

  #ifdef UTERM_BLACK_SCREEN_WORKAROUND
  // Workaround a nasty driver bug where the entire screen turns black if something
  // isn't being redrawn each frame.
  m_attrs.UpdateWith(0, [](Attr &attr) {
    attr.dirty = true;
  });
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
