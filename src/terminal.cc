#include "terminal.h"

#include <algorithm>
#include <unistd.h>

#include <xkbcommon/xkbcommon-keysyms.h>

Terminal::Terminal() {
  tsm_screen_new(&m_screen, nullptr, nullptr);
  tsm_vte_new(&m_vte, m_screen, StaticWrite, static_cast<void*>(this), nullptr, nullptr);
}

void Terminal::set_draw_cb(DrawCb draw_cb) { m_draw_cb = draw_cb; }
void Terminal::set_pty(Pty *pty) { m_pty = pty; }

Pos Terminal::cursor() {
  return {tsm_screen_get_cursor_x(m_screen), tsm_screen_get_cursor_y(m_screen)};
}

Error Terminal::Resize(int x, int y) {
  tsm_screen_resize(m_screen, x, y);
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
  return tsm_vte_handle_keyboard(m_vte, keysym, keysym, mods, TSM_VTE_INVALID);
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
                         uint width, uint posx, uint posy, const tsm_screen_attr *attr,
                         tsm_age_t age, void *data) {
  Terminal *term = static_cast<Terminal*>(data);

  if (term->m_age != 0 && age != 0 && age <= term->m_age) {
    return 0;
  }

  u32string utf32(len, '\0');
  std::copy(chars, chars + len, utf32.begin());

  Pos pos{posx, posy};

  Attr nattr;
  nattr.foreground = SkColorSetRGB(attr->fr, attr->fg, attr->fb);
  nattr.background = SkColorSetRGB(attr->br, attr->bg, attr->bb);
  nattr.flags = 0;
  if (attr->bold) {
    nattr.flags |= Attr::kBold;
  }
  if (attr->underline) {
    nattr.flags |= Attr::kUnderline;
  }
  if (attr->inverse) {
    nattr.flags |= Attr::kInverse;
  }
  if (attr->protect) {
    nattr.flags |= Attr::kProtect;
  }

  term->m_draw_cb(utf32, pos, nattr, width);
  return 0;
}
