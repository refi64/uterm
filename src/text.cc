#include "text.h"

#include <SkPath.h>
#include <SkTextBlob.h>
#include <SkTypeface.h>

FontStyle AttrsToFontStyle(Attr attrs) {
  if (attrs.flags & Attr::kBold) {
    return FontStyle::kBold;
  } else if (attrs.flags & Attr::kItalic) {
    return FontStyle::kItalic;
  } else {
    return FontStyle::kNormal;
  }
}

GlyphRenderer::GlyphRenderer() {
  for (auto &styled_font : m_styled_fonts) {
    styled_font.font.setSubpixel(true);
  }
}

void GlyphRenderer::Resize(int size) {
  m_glyphs.resize(size);
}

void GlyphRenderer::SetTextSize(int height) {
  for (auto &styled_font : m_styled_fonts) {
    styled_font.font.setSize(SkIntToScalar(height));
  }

  UpdateForFontChange();
}

void GlyphRenderer::SetFont(string name) {
  SkFontStyle styles[] = {
    SkFontStyle::Normal(),
    SkFontStyle::Bold(),
    SkFontStyle::Italic(),
  };

  for (int i = 0; i < kStyleEnd; i++) {
    m_styled_fonts[i].font.setTypeface(SkTypeface::MakeFromName(name.c_str(), styles[i]));
  }

  UpdateForFontChange();
}

bool GlyphRenderer::UpdateGlyph(char32_t c, int index, FontStyle style) {
  auto &styled_font = m_styled_fonts[FontStyleToInt(style)];

  if (c < kCharMax) {
    auto glyph = styled_font.glyph_cache[c];
    if (glyph) {
      m_glyphs[index] = glyph;
      return true;
    }
  }

  styled_font.font.textToGlyphs(&c, sizeof(c), kUTF32_SkTextEncoding, &m_glyphs[index], 1);
  return m_glyphs[index] != 0;
}

void GlyphRenderer::ClearGlyph(int index) {
  m_glyphs[index] = m_styled_fonts[kStyleNormal].glyph_cache[' '];
}

int GlyphRenderer::GetHeight() {
  return m_styled_fonts[kStyleNormal].font.getSize() +
         m_styled_fonts[kStyleNormal].metrics.fBottom;
}

int GlyphRenderer::GetWidth() {
  auto &styled_font = m_styled_fonts[kStyleNormal];

  if (styled_font.metrics.fAvgCharWidth) {
    return styled_font.metrics.fAvgCharWidth;
  }

  SkRect bounds;
  const char *s = "x";
  styled_font.font.measureText(s, sizeof(s[0]), kUTF8_SkTextEncoding, &bounds);
  return bounds.width();
}

int GlyphRenderer::GetBaselineOffset() {
  return m_styled_fonts[kStyleNormal].metrics.fBottom;
}

void GlyphRenderer::DrawRange(SkCanvas *canvas, SkPoint *positions, Attr attrs,
                              size_t begin, size_t end, bool is_primary) {
  auto style = AttrsToFontStyle(attrs);
  auto &styled_font = m_styled_fonts[FontStyleToInt(style)];

  auto &font = styled_font.font;
  auto &metrics = styled_font.metrics;
  SkColor color = attrs.flags & Attr::kInverse ? attrs.background : attrs.foreground;

  SkPaint paint;
  paint.setColor(color);
  paint.setBlendMode(SkBlendMode::kSrc);
  paint.setAntiAlias(true);

  SkTextBlobBuilder builder;
  const SkTextBlobBuilder::RunBuffer& run = builder.allocRunPos(font, end - begin);
  std::copy(m_glyphs.data() + begin, m_glyphs.data() + end, run.glyphs);
  // XXX: memcpy
  memcpy(run.pos, positions + begin, (end - begin) * sizeof(positions[0]));

  canvas->drawTextBlob(builder.make(), 0, 0, paint);

  if (is_primary && attrs.flags & Attr::kUnderline) {
    SkScalar last_y = positions[begin].y();
    paint.setStyle(SkPaint::kStroke_Style);

    SkScalar y_offset = 0;
    SkScalar stroke_width = SkIntToScalar(GetHeight()) / 15.0;
    auto &metrics = styled_font.metrics;

    if (metrics.fFlags & SkFontMetrics::kUnderlinePositionIsValid_Flag) {
      y_offset = metrics.fUnderlinePosition;
    }

    if (metrics.fFlags & SkFontMetrics::kUnderlineThicknessIsValid_Flag) {
      stroke_width = metrics.fUnderlineThickness;
    }

    for (size_t i = begin; i < end; ) {
      SkScalar begin_x = positions[i].x();
      while (positions[i].y() == last_y && i < end) {
        i++;
      }
      SkScalar end_x = positions[i - 1].x() + GetWidth();

      SkPath path;
      path.moveTo(begin_x, last_y + y_offset);
      path.lineTo(end_x, last_y + y_offset);

      paint.setStrokeWidth(stroke_width);
      canvas->drawPath(path, paint);

      if (i < end) {
        last_y = positions[i].y();
        i++;
      }
    }
  }
}

void GlyphRenderer::UpdateForFontChange() {
  for (auto &styled_font : m_styled_fonts) {
    styled_font.font.getMetrics(&styled_font.metrics);
  }

  for (auto &styled_font : m_styled_fonts) {
    SkFont &font = styled_font.font;
    auto &glyph_cache = styled_font.glyph_cache;

    for (char c = 0; c < kCharMax; c++) {
      if (isprint(c)) {
        SkGlyphID glyph;
        font.textToGlyphs(&c, sizeof(c), kUTF8_SkTextEncoding, &glyph, 1);
        if (glyph) {
          glyph_cache[c] = glyph;
        }
      }
    }
  }
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

bool TextManager::set_cell(int x, int y, char32_t value) {
  uint index = PosToOffset(x, y);
  if (m_text[index] == value) {
    return false;
  }

  m_text[index] = value;
  return true;
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
                                        Attr attrs, size_t begin, size_t end,
                                        bool is_primary) {
  renderer->DrawRange(canvas, m_positions.data(), attrs, begin, end, is_primary);
}
