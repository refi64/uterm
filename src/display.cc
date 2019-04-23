#include "display.h"

#include <absl/container/inlined_vector.h>

// Clamps v to the range low (inclusive) to high (exclusive).
template <typename T>
T clamp(T v, T low, T high) {
  return v < low ? low : (v > high ? high : v);
}

Display::Display(Terminal *term): m_term{term}, m_attrs{m_term->default_attr()} {
  using namespace std::placeholders;
  m_term->set_draw_cb(std::bind(&Display::TermDraw, this, _1, _2, _3, _4));
}

void Display::AddFont(string name, int size) {
  m_renderers.emplace_back();
  m_renderers.back().SetFont(name);
  m_renderers.back().SetTextSize(size);

  UpdateWidth();
  UpdateGlyphs();
}

void Display::SetSelection(Selection state, int mx, int my) {
  int x = clamp<int>(mx / m_char_width, 0, m_text.cols() - 1);
  int y = clamp<int>(my / m_renderers[0].FindHeight(), 0, m_text.rows() - 1);

  m_term->SetSelection(state, x, y);
}

void Display::EndSelection() {
  m_term->EndSelection();
}

Error Display::Resize(int width, int height) {
  assert(m_char_width != -1);

  int rows = (height - m_renderers[0].FindBaselineOffset()) / m_renderers[0].FindHeight();
  int cols = width / m_char_width;

  m_text.Resize(cols, rows);

  for (auto &renderer : m_renderers) {
    renderer.Resize(rows * cols);
  }
  m_attrs.Resize(rows * cols);

  auto err = m_term->Resize(cols, rows);

  UpdatePositions();

  m_attrs.UpdateWith(0, rows * cols, [](Attr &attr) {
    attr.flags |= Attr::kDirty;
  });
  m_has_updated = true;

  if (err) {
    return err.Extend("while resizing display");
  } else {
    return Error::New();
  }
}

bool Display::Draw(SkCanvas *canvas, bool lazy_updating) {
  bool significant_redraw = m_has_updated;

  if (!significant_redraw && lazy_updating) {
    return false;
  }

  absl::InlinedVector<AttrSet::Span, 64> dirty;
  AttrSet::Span *pspan = nullptr;

  while ((pspan = m_attrs.NextSpan(pspan))) {
    // XXX: should ignore dirty tracking if not lazy updating
    if (!(pspan->data.flags & Attr::kDirty) && lazy_updating) continue;

    bool inverse = pspan->data.flags & Attr::kInverse;
    SkColor background;
    if (pspan->data.flags & Attr::kInverse) {
      background = pspan->data.foreground;
    } else {
      background = pspan->data.background;
    }

    HighlightRange(canvas, m_text.OffsetToPos(pspan->begin),
                   m_text.OffsetToPos(pspan->end), background);

    dirty.push_back(*pspan);
  }

  for (auto &span : dirty) {
    bool is_primary = true;
    for (auto &renderer : m_renderers) {
      m_text.DrawRangeWithRenderer(canvas, &renderer, span.data, span.begin, span.end,
                                   is_primary);
      is_primary = false;
    }

    m_attrs.UpdateWith(span.begin, span.end, [](Attr &attr) {
      attr.flags &= ~Attr::kDirty;
    });
  }

  m_has_updated = false;
  return significant_redraw;
}

void Display::TermDraw(const u32string& str, Pos pos, Attr attr, int width) {
  attr.flags |= Attr::kDirty;

  if (m_text.set_cell(pos.x, pos.y, str[0] ? str[0] : ' ')) {
    UpdateGlyph(pos.x, pos.y);
  }

  m_attrs.Update(m_text.PosToOffset(pos), attr);
  m_has_updated = true;
}

void Display::UpdateWidth() {
  m_char_width = m_renderers[0].FindWidth();
  UpdatePositions();
}

void Display::UpdatePositions() {
  m_text.UpdatePositions(m_renderers[0].FindHeight(), m_char_width);
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
  if (index >= m_attrs.size()) {
    return;
  }

  char32_t c = m_text.cell(x, y);
  FontStyle style = AttrsToFontStyle(m_attrs.At(index));

  bool found_success = false;
  for (auto &renderer : m_renderers) {
    if (found_success) {
      renderer.ClearGlyph(index);
    } else {
      found_success = renderer.UpdateGlyph(c, index, style);
      if (!found_success) {
        renderer.ClearGlyph(index);
      }
    }
  }

  // Fallback to the first renderer if a glyph cannot be found.
  if (!found_success) {
    m_renderers[0].UpdateGlyph(c, index, style);
  }
}

void Display::HighlightRange(SkCanvas *canvas, Pos begin, Pos end, SkColor color) {
  assert(begin.y <= end.y);

  for (int y = begin.y; y <= end.y; y++) {
    int first = y == begin.y ? begin.x : 0,
        last = y == end.y ? end.x : m_text.cols();
    SkRect rect = SkRect::MakeXYWH(m_char_width * first,
                                   m_renderers[0].FindHeight() * y +
                                    m_renderers[0].FindBaselineOffset(),
                                   m_char_width * (last - first),
                                   m_renderers[0].FindHeight());
    canvas->save();
    canvas->clipRect(rect, false);
    canvas->clear(color);
    canvas->restore();
  }
}
