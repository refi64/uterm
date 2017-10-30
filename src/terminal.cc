#include "terminal.h"

#include <algorithm>

Terminal::Terminal() {
  tsm_screen_new(&m_screen, nullptr, nullptr);
  tsm_vte_new(&m_vte, m_screen, StaticWrite, static_cast<void*>(this), nullptr, nullptr);
}

void Terminal::set_draw(DrawCb draw) { m_draw = draw; }
/* void Terminal::set_connection(int fd) { m_fd = fd; } */

void Terminal::WriteToScreen(string text) {
  tsm_vte_input(m_vte, text.c_str(), text.size());
}

void Terminal::Draw() {
  m_age = tsm_screen_draw(m_screen, StaticDraw, static_cast<void*>(this));
}

void Terminal::StaticWrite(tsm_vte *vte, const char *u8, size_t len, void *data) {
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
