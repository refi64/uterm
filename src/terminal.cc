#include "terminal.h"

#include <algorithm>
#include <unistd.h>

Terminal::Terminal() {
  tsm_screen_new(&m_screen, nullptr, nullptr);
  tsm_vte_new(&m_vte, m_screen, StaticWrite, static_cast<void*>(this), nullptr, nullptr);
}

void Terminal::set_draw(DrawCb draw) { m_draw = draw; }
void Terminal::set_pty(Pty *pty) { m_pty = pty; }

void Terminal::WriteToScreen(string text) {
  tsm_vte_input(m_vte, text.c_str(), text.size());
}

Error Terminal::KeyboardToFd(uint32 keysym, int mods) {
  bool ret = tsm_vte_handle_keyboard(m_vte, keysym, keysym, mods, TSM_VTE_INVALID);
  return ret ? Error::New("tsm_vte_handle_keyboard failed") : Error::New();
}

void Terminal::Draw() {
  m_age = tsm_screen_draw(m_screen, StaticDraw, static_cast<void*>(this));
}

void Terminal::StaticWrite(tsm_vte *vte, const char *u8, size_t len, void *data) {
  Terminal *term = static_cast<Terminal*>(data);

  if (term->m_pty != nullptr) {
    if (auto err = term->m_pty->Write(string(u8, len))) {
      fmt::print("WARNING: error writing in StaticWrite: {}\n", err.trace(0));
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

  char utf32_bytes[4] = {0};
  std::copy(chars, chars + len, utf32_bytes);
  string utf32(utf32_bytes, sizeof(utf32_bytes));

  Pos pos;
  pos.x = posx;
  pos.y = posy;

  term->m_draw(utf32, pos);
  return 0;
}
