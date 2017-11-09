#include "terminal.h"

#include <algorithm>
#include <unistd.h>

#include <xkbcommon/xkbcommon-keysyms.h>

Terminal::Terminal(Attr defaults): m_default_attr{defaults} {
  tsm_screen_new(&m_screen, nullptr, nullptr);
  tsm_vte_new(&m_vte, m_screen, StaticWrite, static_cast<void*>(this), nullptr, nullptr);

  assert(!m_default_attr.flags && !m_default_attr.dirty);

  tsm_screen_attr tattr;
  tattr.fccode = tattr.bccode = -1;

  tattr.fr = SkColorGetR(m_default_attr.foreground);
  tattr.fg = SkColorGetG(m_default_attr.foreground);
  tattr.fb = SkColorGetB(m_default_attr.foreground);

  tattr.br = SkColorGetR(m_default_attr.background);
  tattr.bg = SkColorGetG(m_default_attr.background);
  tattr.bb = SkColorGetB(m_default_attr.background);

  tattr.bold = tattr.underline = tattr.inverse = tattr.protect = tattr.blink = 0;
  tsm_screen_set_def_attr(m_screen, &tattr);

  ResetSelection();
}

void Terminal::set_draw_cb(DrawCb draw_cb) { m_draw_cb = draw_cb; }
void Terminal::set_copy_cb(CopyCb copy_cb) { m_copy_cb = copy_cb; }
void Terminal::set_paste_cb(PasteCb paste_cb) { m_paste_cb = paste_cb; }
void Terminal::set_pty(Pty *pty) { m_pty = pty; }

Pos Terminal::cursor() {
  return {tsm_screen_get_cursor_x(m_screen), tsm_screen_get_cursor_y(m_screen)};
}

void Terminal::SetSelection(Selection state, int x, int y) {
  switch (state) {
  case Selection::kBegin:
    ResetSelection();
    tsm_screen_selection_start(m_screen, x, y);
    m_selection_range.begin_x = m_selection_range.end_x = x;
    m_selection_range.begin_y = m_selection_range.end_y = y;
    break;
  case Selection::kUpdate:
    tsm_screen_selection_target(m_screen, x, y);

    if (y < m_selection_range.begin_y ||
        (y == m_selection_range.begin_y && x < m_selection_range.begin_x)) {
      // Moving backwards: update the beginning offsets.
      m_selection_range.end_x = std::max(m_selection_range.begin_x,
                                         m_selection_range.end_x);
      m_selection_range.end_y = std::max(m_selection_range.begin_y,
                                         m_selection_range.end_y);
      m_selection_range.begin_x = x;
      m_selection_range.begin_y = y;
    } else {
      m_selection_range.end_x = x;
      m_selection_range.end_y = y;
    }
    break;
  case Selection::kEnd:
    assert(false);
  }
}

void Terminal::EndSelection() {
  if (m_selection_range.begin_x == m_selection_range.end_x &&
      m_selection_range.begin_y == m_selection_range.end_y) {
    ResetSelection();
  } else {
    char *buf = nullptr;
    uint sz = tsm_screen_selection_copy(m_screen, &buf);
    m_selection_contents = string(buf, sz);
    free(buf);
  }
}

void Terminal::ResetSelection() {
  tsm_screen_selection_reset(m_screen);
  m_selection_range.begin_x = m_selection_range.begin_y = m_selection_range.end_x =
                              m_selection_range.end_y = 0;
  m_selection_contents = "";
}

Error Terminal::Resize(int x, int y) {
  tsm_screen_resize(m_screen, x, y);
  Draw();
  if (auto err = m_pty->Resize(x, y)) {
    return err.Extend("resizing terminal");
  } else {
    return Error::New();
  }
}

void Terminal::WriteToScreen(string text) {
  tsm_vte_input(m_vte, text.c_str(), text.size());
}

bool Terminal::WriteKeysymToPty(uint32 keysym, int mods) {
  if (keysym == XKB_KEY_C && mods & KeyboardModifier::kControl &&
      !m_selection_contents.empty()) {
    m_copy_cb(m_selection_contents);
    return true;
  } else if (keysym == XKB_KEY_V && mods & KeyboardModifier::kControl) {
    auto str = m_paste_cb();
    for (auto c : str) {
      // XXX: This disregards unicode.
      WriteUnicodeToPty(c);
    }
    Draw();
    return true;
  } else {
    return tsm_vte_handle_keyboard(m_vte, keysym, keysym, mods, TSM_VTE_INVALID);
  }
}

bool Terminal::WriteUnicodeToPty(uint32 code) {
  return tsm_vte_handle_keyboard(m_vte, XKB_KEY_NoSymbol, XKB_KEY_NoSymbol, 0, code);
}

void Terminal::Draw() {
  m_age = tsm_screen_draw(m_screen, StaticDraw, static_cast<void*>(this));
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
  attr.foreground = SkColorSetRGB(tattr->fr, tattr->fg, tattr->fb);
  attr.background = SkColorSetRGB(tattr->br, tattr->bg, tattr->bb);
  attr.flags = 0;
  if (tattr->bold) {
    attr.flags |= Attr::kBold;
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
