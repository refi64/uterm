#include "terminal.h"

#include <algorithm>
#include <unistd.h>

#include <utf8.h>

Terminal::Terminal() {
  tsm_screen_new(&m_screen, nullptr, nullptr);
  tsm_vte_new(&m_vte, m_screen, StaticWrite, static_cast<void*>(this), nullptr, nullptr);
  tsm_vte_set_osc_cb(m_vte, StaticOsc, static_cast<void*>(this));

  tsm_screen_attr tattr;
  tattr.fccode = Colors::kForeground;
  tattr.bccode = Colors::kBackground;

  tattr.bold = tattr.underline = tattr.inverse = tattr.protect = tattr.blink = 0;
  tsm_screen_set_def_attr(m_screen, &tattr);

  constexpr int kMaxSb = 1024;
  tsm_screen_set_max_sb(m_screen, kMaxSb);

  ResetSelection();
}

void Terminal::set_theme(const Theme& theme) { m_theme = &theme; }

void Terminal::set_draw_cb(DrawCb draw_cb) { m_draw_cb = draw_cb; }
void Terminal::set_copy_cb(CopyCb copy_cb) { m_copy_cb = copy_cb; }
void Terminal::set_paste_cb(PasteCb paste_cb) { m_paste_cb = paste_cb; }
void Terminal::set_title_cb(TitleCb title_cb) { m_title_cb = title_cb; }
void Terminal::set_pty(Pty *pty) { m_pty = pty; }

Pos Terminal::cursor() {
  return {tsm_screen_get_cursor_x(m_screen), tsm_screen_get_cursor_y(m_screen)};
}

void Terminal::SetSelection(Selection state, uint x, uint y) {
  switch (state) {
  case Selection::kBegin:
    ResetSelection();
    tsm_screen_selection_start(m_screen, x, y);
    m_selection_range.begin = m_selection_range.end = m_selection_range.origin = {x, y};
    break;
  case Selection::kUpdate:
    if (y < m_selection_range.origin.y ||
        (y == m_selection_range.origin.y && x < m_selection_range.origin.x)) {
      // Moving backwards: update the beginning offsets.
      m_selection_range.end = m_selection_range.origin;
      m_selection_range.begin = {x, y};
    } else {
      m_selection_range.end = {x, y};
    }

    tsm_screen_selection_target(m_screen, x, y);
    break;
  case Selection::kEnd:
    assert(false);
  }

  m_has_updated = true;
}

void Terminal::EndSelection() {
  if (m_selection_range.begin == m_selection_range.end) {
    ResetSelection();
    m_has_updated = true;
  } else {
    char *buf = nullptr;
    uint sz = tsm_screen_selection_copy(m_screen, &buf);
    m_selection_contents = string(buf, sz);
    free(buf);
   }
}

void Terminal::ResetSelection() {
  tsm_screen_selection_reset(m_screen);
  m_selection_range.begin = m_selection_range.end = m_selection_range.origin = {0, 0};
  m_selection_contents = "";
  m_has_updated = true;
}

Error Terminal::Resize(int x, int y) {
  tsm_screen_resize(m_screen, x, y);
  m_has_updated = true;

  if (auto err = m_pty->Resize(x, y)) {
    return err.Extend("resizing terminal");
  } else {
    return Error::New();
  }
}

void Terminal::Scroll(ScrollDirection direction, uint distance) {
  switch (direction) {
  case ScrollDirection::kUp:
    tsm_screen_sb_up(m_screen, distance);
    break;
  case ScrollDirection::kDown:
    tsm_screen_sb_down(m_screen, distance);
    break;
  }

  m_has_updated = true;
}

void Terminal::WriteToScreen(string text) {
  tsm_vte_input(m_vte, text.c_str(), text.size());
  m_has_updated = true;
}

bool Terminal::WriteKeysymToPty(uint32 keysym, int mods) {
  if (keysym == XKB_KEY_C && mods & KeyboardModifier::kControl &&
      !m_selection_contents.empty()) {
    // Copy.
    m_copy_cb(m_selection_contents);
    return true;
  } else if (keysym == XKB_KEY_V && mods & KeyboardModifier::kControl) {
    // Paste.
    auto u8 = m_paste_cb();
    u32string u32;
    utf8::utf8to32(u8.begin(), u8.end(), std::back_inserter(u32));

    for (auto c : u32) {
      WriteUnicodeToPty(c);
    }

    m_has_updated = true;
    return true;
  } else if (keysym == XKB_KEY_Up && mods & KeyboardModifier::kShift) {
    Scroll(ScrollDirection::kUp, 1);
    return true;
  } else if (keysym == XKB_KEY_Down && mods & KeyboardModifier::kShift) {
    Scroll(ScrollDirection::kDown, 1);
    return true;
  } else {
    m_has_updated = true;
    return tsm_vte_handle_keyboard(m_vte, keysym, keysym, mods, TSM_VTE_INVALID);
  }
}

bool Terminal::WriteUnicodeToPty(uint32 code) {
  m_has_updated = true;
  return tsm_vte_handle_keyboard(m_vte, XKB_KEY_NoSymbol, XKB_KEY_NoSymbol, 0, code);
}

void Terminal::Draw() {
  if (!m_has_updated) {
    return;
  }

  m_age = tsm_screen_draw(m_screen, StaticDraw, static_cast<void*>(this));
  m_has_updated = false;
}

static SkColor TsmAttrColorCodeToSkColor(const Theme& theme, int code) {
  return theme[std::min(code, Colors::kMax)];
}

int Terminal::StaticDraw(tsm_screen *screen, uint32 id, const uint32 *chars, size_t len,
                         uint width, uint posx, uint posy, const tsm_screen_attr *tattr,
                         tsm_age_t age, void *data) {
  Terminal *term = static_cast<Terminal*>(data);

  if (term->m_age != 0 && age != 0 && age <= term->m_age) {
    return 0;
  }

  u32string utf32(len, '\0');
  std::copy(chars, chars + len, utf32.begin());

  Pos pos{posx, posy};

  Attr attr;

  if (tattr->fccode >= 0) {
    attr.foreground = TsmAttrColorCodeToSkColor(*term->m_theme, tattr->fccode);
  } else {
    attr.foreground = SkColorSetRGB(tattr->fr, tattr->fg, tattr->fb);
  }
  if (tattr->bccode >= 0) {
    attr.background = TsmAttrColorCodeToSkColor(*term->m_theme, tattr->bccode);
  } else {
    attr.background = SkColorSetRGB(tattr->br, tattr->bg, tattr->bb);
  }

  attr.flags = 0;
  if (tattr->bold) {
    attr.flags |= Attr::kBold;
  }
  if (tattr->italic) {
    attr.flags |= Attr::kItalic;
  }
  if (tattr->underline) {
    attr.flags |= Attr::kUnderline;
  }
  if (tattr->inverse) {
    attr.flags |= Attr::kInverse;
  }
  if (tattr->protect) {
    attr.flags |= Attr::kProtect;
  }

  term->m_draw_cb(utf32, pos, attr, width);
  return 0;
}

void Terminal::StaticWrite(tsm_vte *vte, const char *u8, size_t len, void *data) {
  Terminal *term = static_cast<Terminal*>(data);

  string text(u8, len);
  if (term->m_pty != nullptr) {
    if (auto err = term->m_pty->Write(text)) {
      err.Extend("in StaticWrite").Print();
    }
  }
}

void Terminal::StaticOsc(tsm_vte *vte, const char *u8, size_t len, void *data) {
  Terminal *term = static_cast<Terminal*>(data);

  if (u8[0] == '2' && u8[1] == ';') {
    string text(u8 + 2, len - 2);
    term->m_title_cb(text);
  }
}
